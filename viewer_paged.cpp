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
#include "renderer.h"

/* ------------------------------------------------------------------------ */

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

PagedContent::PagedContent(QScrollArea *parent)
    : QWidget(parent)
{
    renderer = nullptr;
    viewport = parent->viewport();

    zoomFactor = 100;
    purgeInvisible = true;  // purge invisible pages to save memory?

    isMoving = false;
    moveTimer = new QTimer(this);
    moveTimer->setSingleShot(true);
    connect(moveTimer, &QTimer::timeout, this, &PagedContent::stoppedMoving);

    // Never shrink smaller than the content
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

PagedContent::~PagedContent()
{
    if (!pages.isEmpty())
        purgeCache();
}

void PagedContent::setRenderer(Renderer *replacement)
{
    if (!pages.isEmpty())
        purgeCache();

    if (replacement != nullptr
        && replacement->mode() == Renderer::PagedContent) {
        renderer = (PagedContentRenderer*)replacement;

        // Prepare the cache
        int numPages = renderer->numPages();
        pages.reserve(numPages);
        for (int i = 0; i < numPages; i++)
            pages.append(new Page);

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
        // Render at the correct physical size on high-DPI screens
        renderer->setPixelDensity(logicalDpiX(), logicalDpiY());
        qreal dpRatio = devicePixelRatio();

        for (int i = 0; i < pages.count(); i++) {
            Page *page = pages[i];
            // The renderer does not understand Qt's high-DPI handling
            // (something it and I have in common), so we need to manually
            // scale this back to the correct logical size
            QSize size = renderer->pageSize(i) / dpRatio;

            page->width = size.width();
            page->height = size.height();

            // Purge the old image so we're forced to re-render
            page->image = QImage();
        }
    }

    fitToContent();
    setPagePositions();
}

void PagedContent::clear()
{
    visiblePages.clear();
    update();
}

void PagedContent::display()
{
    setZoomFactor(zoomFactor);
    refresh();
}

/*
 * Render and paint visible pages.
 */
void PagedContent::refresh()
{
    visiblePages.clear();
    visiblePages.reserve(2);    // this doesn't have to be exact

    QRect visibleArea = visibleRect();
    for (int i = 0; i < pages.count(); i++) {
        Page *page = pages[i];

        if (page->rect().intersects(visibleArea)) {
            if (page->image.isNull()
                && (!(isMoving || page->isRendering))) {
                // Request an image from the renderer
                // setPageImage() will paint it when it comes back
                page->isRendering = true;
                emit imageRequested(i);
            }
            visiblePages.append(pages.at(i));
        } else if (purgeInvisible)
            page->image = QImage(); // tantamount to deletion
        else if (page->y > visibleArea.bottom())
            break;  // the remaining pages are outside our visible area
    }

    update();
}

void PagedContent::moveEvent(QMoveEvent *event)
{
    if (updatesEnabled()) {
        event->accept();

        // Refresh once immediately so the user can see the new content, but
        // delay further renders until we've stopped moving.
        // This avoids rendering pages that aren't visible for any meaningful
        // amount of time when rapidly scrolling.
        refresh();
        isMoving = true;
        moveTimer->start(50);
    } else
        event->ignore();
}

void PagedContent::paintEvent(QPaintEvent *event)
{
    if (updatesEnabled()) {
        event->accept();

        QPainter painter(this);
        for (int i = 0; i < visiblePages.count(); i++) {
            const Page *page = visiblePages.at(i);
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
    } else
        event->ignore();
}

void PagedContent::resizeEvent(QResizeEvent *event)
{
    if (updatesEnabled()) {
        event->accept();

        setPagePositions();
    } else
        event->ignore();
}

/*
 * Adjust this widget's size so it can fit all its content.
 */
void PagedContent::fitToContent()
{
    int w = 0, h = 0, pageCount = pages.count();
    QRect visibleArea = visibleRect();
    bool wereUpdatesEnabled = updatesEnabled();

    if (pageCount) {
        h = (pageCount - 1) * PAGE_MARGIN;
        for (int i = 0; i < pageCount; i++) {
            Page *page = pages[i];
            w = std::max(w, page->width);
            h += page->height;
        }
    }

    // Disable updates so the resize event doesn't call setPagePositions().
    // It isn't reliably triggered here, so we call it manually after calling
    // this method to ensure it always happens when we need it to.
    setUpdatesEnabled(false);
    setMinimumSize(w, h);
    resize(std::max(w, visibleArea.width()), h);
    setUpdatesEnabled(wereUpdatesEnabled);
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
        // Paint at the correct physical size on high-DPI screens
        page->image.setDevicePixelRatio(devicePixelRatio());
        // We only need to repaint this page; the others are fine
        update(page->rect());
    }
}

void PagedContent::stoppedMoving()
{
    isMoving = false;
    refresh();
}
