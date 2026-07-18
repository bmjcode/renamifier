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
#include "render_image.h"

/* ------------------------------------------------------------------------ */

// Margin in pixels for graphical content
#define PAGE_MARGIN 2

// The initial viewer size is 8.5 x 5.5 in, or half of a US letter page.
// This fits a reasonable amount of content without making drastic assumptions
// about the size of the user's screen, and approximates the 16:9 or 16:10
// aspect ratio found on most modern displays.
#define INITIAL_WIDTH 85
#define INITIAL_HEIGHT 55
// Units above are multiplied by a factor of 10 to allow use of integer math.
#define INITIAL_FACTOR 10

/* ------------------------------------------------------------------------ */

struct Page {
    Page();
    inline QRect rect() const { return QRect(x, y, width, height); }
    inline QSize size() const { return QSize(width, height); }

    QPixmap pixmap; // more efficient for display than QImage
    int x;
    int y;
    int width;
    int height;
    bool isRendering;
};

Page::Page()
{
    x = y = width = height = 0;
    isRendering = false;
}

/* ------------------------------------------------------------------------ */

PagedContentViewer::PagedContentViewer(QWidget *parent)
    : QScrollArea(parent)
{
    content = new PagedContent(this);
    setWidget(content);

    setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
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

void PagedContentViewer::setRenderer(Renderer *replacement)
{
    content->setRenderer(replacement);
}

void PagedContentViewer::setZoomFactor(int percent)
{
    content->setZoomFactor(percent);
}

QPoint PagedContentViewer::scrollBarPosition() const {
    return QPoint(
        horizontalScrollBar()->sliderPosition(),
        verticalScrollBar()->sliderPosition());
}

void PagedContentViewer::setScrollBarPosition(int x, int y)
{
    horizontalScrollBar()->setSliderPosition(x);
    verticalScrollBar()->setSliderPosition(y);
}

void PagedContentViewer::clear()
{
    content->clear();
    // Scroll back to the top-left corner
    setScrollBarPosition(0, 0);
}

void PagedContentViewer::display()
{
    content->display();
}

void PagedContentViewer::refresh()
{
    content->refresh();
}

void PagedContentViewer::wheelEvent(QWheelEvent *event)
{
    // Adapted from QPlainTextEdit::wheelEvent()
    if (event->modifiers() & Qt::ControlModifier) {
        float delta = event->angleDelta().y() / 120.f;
        emit wheelZoomed(delta);
        return;
    }
    QScrollArea::wheelEvent(event);
}

/* ------------------------------------------------------------------------ */

PagedContent::PagedContent(QScrollArea *parent)
    : QWidget(parent)
{
    renderer = nullptr;
    movie = nullptr;
    zoomFactor = 100;

    renderTimer = new QTimer(this);
    renderTimer->setSingleShot(true);
    connect(renderTimer, &QTimer::timeout,
            this, &PagedContent::renderVisiblePages);

    // We manage this widget's size manually in updatePageGeometry()
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

PagedContent::~PagedContent()
{
    purgeCache();
}

void PagedContent::setRenderer(Renderer *replacement)
{
    if (!pages.isEmpty())
        purgeCache();

    renderer = qobject_cast<PagedContentRenderer*>(replacement);
    if (renderer == nullptr)
        return;

    // Prepare the cache
    int numPages = renderer->numPages();
    pages.reserve(numPages);
    for (int i = 0; i < numPages; i++)
        pages.append(new Page);

    ImageRenderer *imageRenderer = qobject_cast<ImageRenderer*>(renderer);
    if (imageRenderer != nullptr && imageRenderer->supportsAnimation()) {
        // Bypass normal rendering and let QMovie provide the images.
        // Even though animations and paged content are logically distinct
        // things, their viewers would share enough layout and painting
        // logic anyway that it makes practical sense to combine them.
        movie = new QMovie(imageRenderer->path());
        connect(movie, &QMovie::updated,
                this, &PagedContent::showNextFrame);
    } else {
        connect(this, &PagedContent::imageRequested,
                renderer, &PagedContentRenderer::renderPage);
        connect(renderer, &PagedContentRenderer::renderedPage,
                this, &PagedContent::setPageImage);
    }
}

void PagedContent::setZoomFactor(int percent)
{
    zoomFactor = percent;
    display();
}

void PagedContent::clear()
{
    purgeCache();
    resize(0, 0);
}

void PagedContent::display()
{
    // Temporarily disable updates so resizing in updatePageGeometry()
    // doesn't force an extra repaint
    bool wereUpdatesEnabled = updatesEnabled();
    setUpdatesEnabled(false);
    updatePageGeometry();
    setUpdatesEnabled(wereUpdatesEnabled);
    refresh();
}

void PagedContent::refresh()
{
    if (movie != nullptr)
        movie->stop();

    checkVisiblePages();
    update();

    if (movie != nullptr)
        movie->start();
}

void PagedContent::moveEvent(QMoveEvent *event)
{
    if (!updatesEnabled() || pages.isEmpty())
        return;

    refresh();
}

void PagedContent::paintEvent(QPaintEvent *event)
{
    if (!updatesEnabled() || visiblePages.isEmpty())
        return;

    QPainter painter(this);
    QRegion eventRegion = event->region();

    for (int i = 0; i < visiblePages.count(); i++) {
        const Page *page = pages.at(visiblePages.at(i));
        QRect pageRect = page->rect();

        // The area to paint may be smaller than the total visible area
        if (eventRegion.intersects(pageRect)) {
            if (page->pixmap.isNull()) {
                // Paint a placeholder to reduce flicker
                if (renderer->shouldPaintPlaceholders())
                    painter.fillRect(pageRect, Qt::white);
            } else
                painter.drawPixmap(pageRect, page->pixmap);
        }
    }
}

/*
 * Determine which pages are currently visible, and render them if needed.
 */
void PagedContent::checkVisiblePages()
{
    visiblePages.clear();
    visiblePages.reserve(2);    // this doesn't have to be exact

    QRegion vRegion = visibleRegion();
    int bottom = vRegion.boundingRect().bottom();

    for (int i = 0; i < pages.count(); i++) {
        Page *page = pages[i];

        if (vRegion.intersects(page->rect()))
            visiblePages.append(i);
        else
            page->pixmap = QPixmap();   // tantamount to deletion
    }

    // We trigger rendering via a timer to combine multiple calls occurring
    // in quick succession when rapidly scrolling. This prevents rendering
    // pages that aren't visible for any meaningful amount of time.
    if (!visiblePages.isEmpty())
        renderTimer->start(10);
}

void PagedContent::purgeCache()
{
    if (movie != nullptr) {
        movie->stop();
        delete movie;       // a hard delete is safe here, and needed to not
                            // block renaming the file on Windows
        movie = nullptr;
    }

    for (int i = 0; i < pages.count(); i++)
        delete pages[i];
    pages.clear();
    pages.squeeze();
    visiblePages.clear();   // this is small, so we don't need to squeeze() it
}

/*
 * Calculate page sizes and layouts at our current zoom level and screen DPI.
 */
void PagedContent::updatePageGeometry()
{
    if (renderer == nullptr)
        return;

    renderer->setZoomFactor(zoomFactor);
    // Images are always rendered at their real pixel size, but layout
    // calculations use DPI-independent "logical" pixels. You are not
    // expected to understand this -- just to trust that this produces
    // correct results on high-DPI screens.
    qreal dpRatio = devicePixelRatio();
    int dpiX = logicalDpiX(), dpiY = logicalDpiY();
    if (!renderer->isPixelExact())
        // Scale the image based on inches, not pixels, so it fills
        // the same relative area on screen regardless of DPI
        dpiX *= dpRatio, dpiY *= dpRatio;
    renderer->setPixelDensity(dpiX, dpiY);

    // Now the actual page size calculations
    int pageCount = pages.count(),
        maxWidth = 0,
        totalHeight = std::max(0, (pageCount - 1) * PAGE_MARGIN);

    for (int i = 0, y = 0; i < pageCount; i++) {
        Page *page = pages[i];

        // Remember layout uses logical pixels, so dividing by dpRatio
        // is correct here even though I think it looks wrong
        QSize size = renderer->pageSize(i) / dpRatio;
        page->width = size.width();
        page->height = size.height();

        // Since we only support one hardcoded page layout, we can also
        // do those calculations now and avoid needing a separate for-loop
        page->y = y;
        maxWidth = std::max(maxWidth, page->width);
        totalHeight += page->height;
        y += page->height + PAGE_MARGIN;

        // Purge the old image so we're forced to re-render
        page->pixmap = QPixmap();
    }

    // Now that we know the width of the widest page, we can offset
    // smaller pages to center them horizontally
    for (int i = 0; i < pageCount; i++) {
        Page *page = pages[i];
        page->x = std::max(0, (maxWidth - page->width) / 2);
    }

    // Shrink this widget to fit its contents exactly, and let the parent
    // QScrollArea worry about centering it in the viewport
    resize(maxWidth, totalHeight);
}

void PagedContent::renderVisiblePages()
{
    for (int i = 0; i < visiblePages.count(); i++) {
        int num = visiblePages.at(i);
        Page *page = pages[num];

        if (page->pixmap.isNull() && !page->isRendering) {
            // Request an image from the renderer
            // setPageImage() will paint it when it comes back
            page->isRendering = true;
            emit imageRequested(num);
        }
    }
}

void PagedContent::setPageImage(int num, const QImage &image)
{
    if (0 <= num && num < pages.count()) {
        Page *page = pages[num];
        page->pixmap = QPixmap::fromImage(image);
        page->isRendering = false;
        // Paint at the correct physical size on high-DPI screens
        page->pixmap.setDevicePixelRatio(devicePixelRatio());
        // We only need to repaint this page; the others are fine
        update(page->rect());
    }
}

/*
 * Show the next frame of an animation.
 */
void PagedContent::showNextFrame(const QRect &rect)
{
    if (pages.isEmpty() || movie == nullptr)
        return;

    // Animations are always effectively single-"page" documents
    Page *page = pages[0];
    // Since we bypassed the renderer we have to handle scaling ourselves
    page->pixmap = movie->currentPixmap().scaled(
        page->size() * devicePixelRatio(),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    update(page->rect());
}
