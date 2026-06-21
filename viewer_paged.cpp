/*
 * Widget for viewing paged content like a PDF document.
 * Copyright (c) 2021-2026 Benjamin Johnson
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

#include <algorithm>    // for std::max()

#include <QtCore>
#include <QtWidgets>

#include "viewer_paged.h"

/* ------------------------------------------------------------------------ */

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
    inline QRect rect() const { return QRect(x, y, width, height); }

    QImage image;
    int x;
    int y;
    int width;
    int height;
    bool isRendering;
};

Page::Page()
{
    x = y = -1;
    width = height = 0;
    isRendering = false;
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

PagedContent::PagedContent(PagedContentViewer *parent)
    : QFrame(parent)
{
    renderer = nullptr;
    viewport = parent->viewport();

    zoomFactor = 100;

    isMoving = false;
    moveTimer = new QTimer(this);
    moveTimer->setSingleShot(true);
    connect(moveTimer, &QTimer::timeout, this, &PagedContent::stoppedMoving);

    // Never shrink smaller than the content
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

PagedContent::~PagedContent()
{
    clear();
}

void PagedContent::setRenderer(Renderer *replacement)
{
    purgeCache();
    if (replacement->mode() == Renderer::PagedContent) {
        renderer = (PagedContentRenderer*)replacement;

        // Prepare the cache
        int numPages = renderer->numPages();
        pages.reserve(numPages);
        for (int i = 0; i < numPages; i++)
            pages.append(new Page);

        // Set initial state now that the cache is prepared
        setZoomFactor(zoomFactor);

        // Connect signals last now that our prep work is all done
        connect(this, &PagedContent::imageRequested,
                renderer, &PagedContentRenderer::renderPage);
        connect(renderer, &PagedContentRenderer::renderedPage,
                this, &PagedContent::setPageImage);
    } else
        renderer = nullptr;
}

void PagedContent::setZoomFactor(int percent)
{
    zoomFactor = percent;

    if (renderer != nullptr) {
        renderer->setZoomFactor(percent);
        // Logical DPI provides correctly-scaled output on high-DPI screens
        renderer->setPixelDensity(logicalDpiX(), logicalDpiY());

        for (int i = 0; i < pages.count(); i++) {
            Page *page = pages[i];
            QSize size = renderer->pageSize(i);

            page->width = size.width();
            page->height = size.height();
        }
    }

    fitToContent();
    setPagePositions();
}

void PagedContent::clear()
{
    purgeCache();
    refresh();
}

void PagedContent::refresh()
{
    visiblePages.clear();
    visiblePages.reserve(2);    // this doesn't have to be exact

    QRect visibleArea = visibleRect();
    for (int i = 0; i < pages.count(); i++) {
        Page *page = pages[i];

        if (page->rect().intersects(visibleArea)) {
            visiblePages.append(page);
            if (page->image.isNull()
                && (!(isMoving || page->isRendering))) {
                // Request an image from the renderer
                // setPageImage() will paint it when it comes back
                page->isRendering = true;
                emit imageRequested(i);
            }
        } else
            page->image = QImage(); // purge invisible pages to save memory
    }

    update();
}

void PagedContent::moveEvent(QMoveEvent *event)
{
    // Refresh once immediately so the user can see the new content, but
    // delay further renders until we've stopped moving.
    // This avoids rendering pages that aren't visible for any meaningful
    // amount of time when rapidly scrolling.
    refresh();
    isMoving = true;
    moveTimer->start(50);
}

void PagedContent::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    for (int i = 0; i < visiblePages.count(); i++) {
        Page *page = visiblePages[i];
        QRect pageRect = page->rect();

        // The area to paint may be smaller than the total visible area
        if (pageRect.intersects(event->rect())) {
            if (page->image.isNull())
                // Paint a placeholder to reduce flicker
                painter.fillRect(pageRect, Qt::white);
            else
                painter.drawImage(pageRect, page->image);
        }
    }
}

void PagedContent::resizeEvent(QResizeEvent *event)
{
    setPagePositions();
}

/*
 * Adjust this widget's size so it can fit all its content.
 */
void PagedContent::fitToContent()
{
    int w = 0, h = 0, pageCount = pages.count();
    QRect visibleArea = visibleRect();

    if (pageCount) {
        h = (pageCount - 1) * PAGE_MARGIN;
        for (int i = 0; i < pageCount; i++) {
            Page *page = pages[i];
            w = std::max(w, page->width);
            h += page->height;
        }
    }

    setMinimumSize(w, h);
    resize(std::max(w, visibleArea.width()), h);
}

void PagedContent::purgeCache()
{
    for (int i = 0; i < pages.count(); i++)
        delete pages[i];
    pages.clear();
    pages.squeeze();
    visiblePages.clear();   // this is small so we don't need to squeeze() it
}

/*
 * Recalculate page positions when the widget is resized.
 */
void PagedContent::setPagePositions()
{
    QRect visibleArea = visibleRect();

    for (int i = 0, y = 0; i < pages.count(); i++) {
        Page *page = pages[i];
        // Center the page if the visible area is wider
        page->x = std::max(0, (visibleArea.width() - page->width) / 2);
        page->y = y;
        y += page->height + PAGE_MARGIN;
    }
}

void PagedContent::setPageImage(int num, const QImage &image)
{
    if (0 <= num && num < pages.count()) {
        Page *page = pages[num];
        page->image = image;
        page->isRendering = false;
        // Paint just this page
        update(page->rect());
    }
}

void PagedContent::stoppedMoving()
{
    isMoving = false;
    refresh();
}
