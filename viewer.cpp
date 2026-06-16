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
#include <utility>

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
#define PAGE_MARGIN 2

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
    int pageCount;

    clear();
    // Always use logical DPI for correctly-scaled output on high-DPI screens
    renderer = Renderer::create(path, logicalDpiX(), logicalDpiY());
    renderer->moveToThread(renderThread);

    pageCount = renderer->numPages();
    pagedContent->reservePages(pageCount);

    // Calculate the content area
    for (int i = 0; i < pageCount; i++)
        pagedContent->setPageSize(i, renderer->pageSize(i));
    pagedContent->recalculateArea();

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
    pagedContent->setPageImage(num, image);
    pagedContent->update();
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
    recalculateArea();
    update();
}

void PagedContent::recalculateArea()
{
    int w = 0, h = 0, pageCount = pages.count();

    if (pageCount) {
        h = (pageCount - 1) * PAGE_MARGIN;
        for (int i = 0; i < pageCount; i++) {
            Page *page = pages[i];
            w = std::max(w, page->width);
            h += page->height;
        }
    }

    setMinimumSize(w, h);
    resize(w, h);
}

void PagedContent::reservePages(int numPages)
{
    clear();
    pages.reserve(numPages);
    for (int i = 0; i < numPages; i++)
        pages.append(new Page);
}

void PagedContent::setPageImage(int num, const QImage &image)
{
    if (0 <= num && num < pages.count()) {
        Page *page = pages[num];
        page->image = image;
        page->isRendering = false;
    }
}

void PagedContent::setPageSize(int num, const QSize &size)
{
    if (0 <= num && num < pages.count()) {
        Page *page = pages[num];
        page->width = size.width();
        page->height = size.height();
    }
}

void PagedContent::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int x, y;
    Page *page;
    QRect pageRect;
    QList<std::pair<QRect, QImage> > pagesToPaint;

    pagesToPaint.reserve(pages.count());
    for (int i = 0, y = 0; i < pages.count(); i++) {
        page = pages[i];
        // center the page if the widget is wider
        x = std::max(0, (event->rect().width() - page->width) / 2);
        pageRect = QRect(x, y, page->width, page->height);
        if (event->region().intersects(pageRect)) {
            if (page->image.isNull()) {
                // no image for this page; request one from the renderer
                if (!page->isRendering) {
                    page->isRendering = true;
                    emit pageRequested(i);
                }
            } else
                pagesToPaint.append(
                    std::pair<QRect, QImage>(pageRect, page->image));
        } else if (y > event->rect().bottom())
            break;  // the remaining pages are outside the visible area
        y += page->height + PAGE_MARGIN;
    }

    for (int i = 0; i < pagesToPaint.count(); i++)
        painter.drawImage(pagesToPaint[i].first, pagesToPaint[i].second);
}
