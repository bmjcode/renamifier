/*
 * A widget to display file previews.
 * Copyright (c) 2021 Benjamin Johnson
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <algorithm>

#include <QtCore>
#include <QtWidgets>

#include "viewer.h"

// The initial viewer size is 8.5 x 5.5 in, or half of a US letter page.
// This fits a reasonable amount of content without making drastic assumptions
// about the size of the user's screen, and approximates the 16:9 or 16:10
// aspect ratio found on most modern displays.
#define INITIAL_WIDTH 85
#define INITIAL_HEIGHT 55
// Units above are multiplied by a factor of 10 to allow use of integer math.
#define INITIAL_FACTOR 10

// Margin in pixels for graphical content
#define PAGE_MARGIN 1

// Margin in pixels for plain-text content
#define TEXT_MARGIN 8

/* ------------------------------------------------------------------------ */

Viewer::Viewer(QWidget *parent)
    : QStackedWidget(parent)
{
    textContentViewer = new TextContentViewer(this);
    addWidget(textContentViewer);

    pagedContentViewer = new PagedContentViewer(this);
    addWidget(pagedContentViewer);

    // QThread is responsible for this while it exists
    renderer = nullptr;
    renderThread = nullptr;
}

void Viewer::display(const QString &path)
{
    clear();
    renderThread = new QThread;
    // Always use logical DPI for correctly-scaled output on high-DPI screens
    renderer = new Renderer(path, logicalDpiX(), logicalDpiY());
    renderer->moveToThread(renderThread);

    // QThread signals
    connect(renderThread, &QThread::started,
            renderer, &Renderer::render);
    connect(renderThread, &QThread::finished,
            renderer, &QObject::deleteLater);
    connect(renderThread, &QThread::finished,
            renderThread, &QObject::deleteLater);

    // Renderer signals
    connect(renderer, &Renderer::renderedImage,
            this, &Viewer::addImage);
    connect(renderer, &Renderer::renderedPage,
            this, &Viewer::addPage);
    connect(renderer, &Renderer::renderedText,
            this, &Viewer::addText);
    connect(renderer, &Renderer::renderMode,
            this, &Viewer::setRenderMode);
    connect(renderer, &Renderer::renderProgress,
            this, &Viewer::renderProgress);

    renderThread->start();
}

void Viewer::setFocusPolicy(Qt::FocusPolicy policy)
{
    textContentViewer->setFocusPolicy(policy);
    pagedContentViewer->setFocusPolicy(policy);
}

void Viewer::clear()
{
    // Cancel any active rendering operation
    stopRender();

    textContentViewer->clear();
    pagedContentViewer->clear();
}

void Viewer::stopRender()
{
    // renderer and renderThread are cleaned up by QObject::deleteLater()
    if (renderer != nullptr) {
        // Don't respond to any more signals from this Renderer
        disconnect(renderer, nullptr, nullptr, nullptr);
        renderer = nullptr;
    }
    if (renderThread != nullptr) {
        if (renderThread->isRunning())
            renderThread->requestInterruption();
        // Trust, but verify
        renderThread->quit();
        renderThread->wait();
        renderThread = nullptr;
    }
}

void Viewer::addImage(const QImage &image)
{
    pagedContentViewer->addImage(image);
}

void Viewer::addPage(const QImage &image)
{
    pagedContentViewer->addPage(image);
}

void Viewer::addText(const QString &text)
{
    if (currentWidget() == textContentViewer) {
        // FIXME: This clobbers any existing content.
        textContentViewer->setPlainText(text);
    } else if (currentWidget() == pagedContentViewer)
        pagedContentViewer->addText(text);
}

void Viewer::setRenderMode(int mode)
{
    if (mode == Renderer::TextContent)
        setCurrentWidget(textContentViewer);
    else if (mode == Renderer::PagedContent)
        setCurrentWidget(pagedContentViewer);
}

/*
 * Default to PagedContentViewer's preferred size.
 */
QSize Viewer::sizeHint() const
{
    return pagedContentViewer->sizeHint();
}

/* ------------------------------------------------------------------------ */

TextContentViewer::TextContentViewer(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setReadOnly(true);
    setTabChangesFocus(true);
    setWordWrapMode(QTextOption::WordWrap);
}

/* ------------------------------------------------------------------------ */

PagedContentViewer::PagedContentViewer(QWidget *parent)
    : QScrollArea(parent)
{
    frame = new QFrame(this);
    layout = new QVBoxLayout(frame);
    setWidget(frame);
    setBackgroundRole(QPalette::Dark);

    // Total width and height of graphical elements in pageWidgets
    // Note: Storing these is an optimization; since they contain fixed
    // images, their dimensions don't change. Text widgets are not included
    // since they are resized dynamically to fit the viewport.
    totalPageWidth = 0;
    totalPageHeight = 0;
}

PagedContentViewer::~PagedContentViewer()
{
    clear();
}

/*
 * Add an image.
 */
void PagedContentViewer::addImage(const QImage &image)
{
    addPage_(image, false);
}

/*
 * Add a page from a multi-page document.
 */
void PagedContentViewer::addPage(const QImage &image)
{
    addPage_(image);
}

/*
 * Add text content.
 *
 * This is mostly useful if you need to mix text and paged content,
 * for example to display an error message if the renderer was unable
 * to process a particular page.
 *
 * QLabel is inefficient for displaying large amounts of text content,
 * which is why TextContentViewer exists for that.
 */
void PagedContentViewer::addText(const QString &text)
{
    QLabel *textWidget = createContentWidget();
    textWidget->setMargin(TEXT_MARGIN);
    textWidget->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    textWidget->setTextFormat(Qt::PlainText);
    textWidget->setText(text);
    textWidget->setWordWrap(true);

    layout->addWidget(textWidget);
    textWidgets.append(textWidget);

    resizeFrame();
}

void PagedContentViewer::clear()
{
    for (int i = 0; i < pageWidgets.size(); ++i) {
        layout->removeWidget(pageWidgets[i]);
        delete pageWidgets[i];
    }
    for (int i = 0; i < textWidgets.size(); ++i) {
        layout->removeWidget(textWidgets[i]);
        delete textWidgets[i];
    }
    pageWidgets.clear();
    textWidgets.clear();

    totalPageWidth = 0;
    totalPageHeight = 0;
    frame->setMinimumSize(0, 0);
    frame->resize(0, 0);
}

/*
 * Default to a size large enough to show a reasonable amount of content on
 * most screens. The exact size is specified by INITIAL_{HEIGHT,WIDTH} above.
 */
QSize PagedContentViewer::sizeHint() const
{
    int initialWidth, initialHeight;
    initialWidth = INITIAL_WIDTH * logicalDpiX() / INITIAL_FACTOR;
    initialHeight = INITIAL_HEIGHT * logicalDpiY();

    // Compensate for the viewport margins and vertical scroll bar
    QMargins margins = viewportMargins();
    initialWidth += margins.left() + margins.right();
    initialWidth += verticalScrollBar()->width();

    return QSize(initialWidth, initialHeight);
}

/*
 * Create a widget to display content.
 */
QLabel *PagedContentViewer::createContentWidget(bool drawBorder)
{
    QLabel *widget = new QLabel(frame);
    if (drawBorder) {
        widget->setBackgroundRole(QPalette::Base);
        widget->setAutoFillBackground(true);
        widget->setFrameStyle(QFrame::Box | QFrame::Plain);
        widget->setLineWidth(1);
        widget->setMargin(PAGE_MARGIN);
    }

    return widget;
}

/*
 * Add a widget displaying graphical content.
 *
 * Used by addImage() and addPage(). The only difference between the two is
 * pages are displayed with a border around their content, and images are not.
 * This matches the format typically used by other document and image viewers,
 * respectively.
 */
void PagedContentViewer::addPage_(const QImage &image, bool drawBorder)
{
    QLabel *pageWidget = createContentWidget(drawBorder);
    pageWidget->setPixmap(QPixmap::fromImage(image));
    pageWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout->addWidget(pageWidget);
    pageWidgets.append(pageWidget);

    // Store the width of the widest page, and the total height of all pages
    QSize pageSize = pageWidget->minimumSizeHint();
    totalPageWidth = std::max(totalPageWidth,
                              pageSize.width() + 2 * PAGE_MARGIN);
    totalPageHeight += pageSize.height() + 2 * PAGE_MARGIN;
    resizeFrame();
}

/*
 * Resize the inner frame when the widget's size changes.
 */
void PagedContentViewer::resizeEvent(QResizeEvent *event)
{
    resizeFrame();
}

/*
 * Resize the inner frame to fit displayed content.
 *
 * The frame's size is NOT updated automatically when we add content,
 * so this is necessary to ensure that content is visible, and that the
 * scrollable area is sized appropriately.
 */
void PagedContentViewer::resizeFrame()
{
    int width, height;

    // The frame should be as wide as the viewport or the widest page,
    // whichever is larger. Note that text widgets are always frame width.
    width = std::max(viewport()->width(), totalPageWidth);

    // The frame should be tall enough to fit all of the displayed content.
    // Images and paged content have fixed heights.
    height = totalPageHeight;
    // Text widgets' height must be dynamically calculated since it may
    // change due to word wrapping.
    // Note QLabel::heightForWidth() does not account for margins.
    for (int i = 0; i < textWidgets.size(); ++i)
        height += textWidgets[i]->heightForWidth(width) + 2 * TEXT_MARGIN;

    frame->setMinimumSize(width, height);
    frame->resize(width, height);
}
