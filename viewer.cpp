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

struct Page {
    Page();

    QImage image;
    int width;
    int height;
    bool isRendering;
};

Page::Page()
{
    width = height = 0;
    isRendering = false;
}

/* ------------------------------------------------------------------------ */

Viewer::Viewer(QWidget *parent)
    : QStackedWidget(parent)
{
    textContentViewer = new TextContentViewer(this);
    addWidget(textContentViewer);

    pagedContentViewer = new PagedContentViewer(this);
    addWidget(pagedContentViewer);

    pagedContent = new PagedContent(pagedContentViewer);
    pagedContentViewer->setWidget(pagedContent);

    renderer = nullptr;
    renderThread = new QThread(this);
    renderThread->start();
}

Viewer::~Viewer()
{
    clear();
    if (renderThread != nullptr) {
        renderThread->quit();
        renderThread->wait();
    }
}

void Viewer::display(const QString &path)
{
    int w, h, pageCount;

    clear();
    // Always use logical DPI for correctly-scaled output on high-DPI screens
    renderer = Renderer::create(path, logicalDpiX(), logicalDpiY());
    renderer->moveToThread(renderThread);

    pageCount = renderer->numPages();
    pagedContent->reservePages(pageCount);

    // Calculate the content area
    w = 0;
    h = 2 * (pageCount - 1) * PAGE_MARGIN;
    for (int i = 0; i < pageCount; i++) {
        QSize pageSize = renderer->pageSize(i);
        pagedContent->setPageSize(i, pageSize.width(), pageSize.height());
        // The total width is that of the widest page
        w = std::max(w, pageSize.width());
        h += pageSize.height();
    }
    pagedContent->setContentSize(w, h);

    // Renderer signals
    connect(renderer, &Renderer::renderedPage,
            this, &Viewer::addPage);
    connect(renderer, &Renderer::renderedText,
            this, &Viewer::addText);
    connect(renderer, &Renderer::renderMode,
            this, &Viewer::setRenderMode);

    // PagedContent signals
    connect(pagedContent, &PagedContent::pageRequested,
            renderer, &Renderer::renderPage);

    QTimer::singleShot(0, renderer, &Renderer::render);
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
    pagedContent->clear();
}

void Viewer::stopRender()
{
    if (renderer != nullptr) {
        // Don't respond to any more signals from this Renderer
        disconnect(renderer, nullptr, nullptr, nullptr);
        delete renderer;
        renderer = nullptr;
    }
}

void Viewer::addPage(int num, const QImage &image)
{
    pagedContent->addPage(num, image);
}

void Viewer::addText(const QString &text)
{
    // FIXME: This clobbers any existing content.
    textContentViewer->setPlainText(text);
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
    setBackgroundRole(QPalette::Dark);
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
 * Resize the inner frame when the widget's size changes.
 */
void PagedContentViewer::resizeEvent(QResizeEvent *event)
{
    widget()->resize(
        std::max(viewport()->width(), widget()->minimumWidth()),
        std::max(viewport()->height(), widget()->minimumHeight()));
}

/* ------------------------------------------------------------------------ */

PagedContent::PagedContent(QWidget *parent)
    : QFrame(parent)
{
}

PagedContent::~PagedContent()
{
    clear();
}

void PagedContent::clear()
{
    for (int i = 0; i < pages.count(); i++)
        delete pages[i];
    pages.clear();
    setContentSize(0, 0);
    update();
}

void PagedContent::reservePages(int numPages)
{
    clear();
    pages.reserve(numPages);
    for (int i = 0; i < numPages; i++)
        pages.append(new Page);
}

void PagedContent::setContentSize(int w, int h)
{
    contentSize_ = QSize(w, h);
    setMinimumSize(contentSize_);
    resize(contentSize_);
}

void PagedContent::setPageSize(int num, int w, int h)
{
    if (num < pages.count()) {
        Page *page = pages[num];
        page->width = w;
        page->height = h;
    }
}

/*
 * Add a page from a multi-page document.
 */
void PagedContent::addPage(int num, const QImage &image)
{
    if (num < pages.count()) {
        Page *page = pages[num];
        page->image = image;
        page->isRendering = false;
    }
    update();
}

void PagedContent::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int x, y, paintedPages = 0;
    Page *page;
    QRect pageRect;

    painter.setClipRegion(event->region());
    for (int i = 0, y = 0; i < pages.count(); i++) {
        page = pages[i];
        // Center the image
        x = std::max(0, (event->rect().width() - page->width) / 2);

        pageRect = QRect(x, y, page->width, page->height);
        if (event->region().intersects(pageRect)) {
            if (page->image.isNull()) {
                // We don't have an image for this page, so tell the renderer
                // to produce one and addPage() will repaint when it's ready
                if (!page->isRendering) {
                    page->isRendering = true;
                    emit pageRequested(i);
                }
                break;
            }
            painter.drawImage(x, y, page->image);
            paintedPages++;
        } else if (paintedPages > 0)
            break;  // we're done painting

        y += page->height + 2 * PAGE_MARGIN;
    }
}
