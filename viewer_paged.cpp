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

    // QPixmap is more efficient for display than QImage
    QPixmap pixmap;
    // All geometry here is in logical pixels
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
    m_fitToWidth = false;

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
    content->setMaximumWidth(
        m_fitToWidth ? viewport()->width() : QWidget::maximumWidth());
    content->display();
}

void PagedContentViewer::setFitToWidth(bool enabled)
{
    m_fitToWidth = enabled;
    display();
}

void PagedContentViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    // If fit-to-width is enabled, call display() again to re-fit the content.
    // Note calling this directly during the resize event would result in a
    // black screen, but with the timer it's actually called slightly after.
    if (m_fitToWidth)
        QTimer::singleShot(0, this, &PagedContentViewer::display);
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
    m_zoomFactor = 100;
    paintPlaceholders = false;

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

    // Placeholders can be helpful for long, relatively slow-to-render
    // files like PDF documents, but can cause flicker for other things
    // like images with transparent backgrounds. We trust the renderer
    // to know what's best for its specific file types.
    paintPlaceholders = renderer->shouldPaintPlaceholders();

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
    m_zoomFactor = percent;
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
    paintVisiblePages();
}

void PagedContent::moveEvent(QMoveEvent *event)
{
    if (!updatesEnabled() || pages.isEmpty())
        return;

    paintVisiblePages();
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
                if (paintPlaceholders)
                    painter.fillRect(pageRect, Qt::white);
            } else {
                QPixmap pixmap;
                // The cached image may need resizing when fit-to-width is
                // enabled or when zooming an animation
                QSize desiredSize = page->size() * devicePixelRatio();
                if (page->pixmap.size() == desiredSize)
                    pixmap = page->pixmap;
                else
                    pixmap = page->pixmap.scaled(desiredSize,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation);
                painter.drawPixmap(pageRect, pixmap);
            }
        }
    }
}

void PagedContent::paintVisiblePages()
{
    if (movie != nullptr)
        movie->stop();

    visiblePages.clear();
    visiblePages.reserve(2);    // this doesn't have to be exact

    // Check which pages are visible, and purge cached pixmaps for those
    // that aren't to save memory
    QRegion vRegion = visibleRegion();
    for (int i = 0; i < pages.count(); i++) {
        Page *page = pages[i];
        if (vRegion.intersects(page->rect()))
            visiblePages.append(i);
        else
            page->pixmap = QPixmap();
    }

    // Immediately paint pages we've previously rendered
    update();

    // Now render any remaining pages. We trigger this via a timer to combine
    // multiple calls occurring in quick succession, which avoids rendering
    // pages that are only momentarily visible when rapidly scrolling
    if (!visiblePages.isEmpty())
        renderTimer->start(10);

    if (movie != nullptr)
        movie->start();
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

    renderer->setZoomFactor(m_zoomFactor);
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
        maxWidgetWidth = maximumWidth(),
        widestPageWidth = 0,
        totalHeight = std::max(0, (pageCount - 1) * PAGE_MARGIN);

    for (int i = 0, y = 0; i < pageCount; i++) {
        Page *page = pages[i];
        QSize size = renderer->pageSize(i); // in physical pixels

        // If the cached pixmap is already the right size, keep it to avoid
        // unnecessary re-rendering; otherwise, purge it. Note we check this
        // before the fit-to-width calculations because it's more efficient
        // to cache the image at full size and shrink it ourselves as needed
        if (page->pixmap.size() != size)
            page->pixmap = QPixmap();

        size /= dpRatio;    // convert to logical pixels for layout math

        // Technically we always fit our content to the widget's width, and
        // the feature called "fit-to-width" simply reduces the maximum widget
        // width from Qt's default; see PagedContentViewer::display()
        if (size.width() > maxWidgetWidth)
            size.scale(maxWidgetWidth, size.height(), Qt::KeepAspectRatio);
        page->width = size.width();
        page->height = size.height();

        // Since we only support one hardcoded page layout, we can also
        // do those calculations now and avoid needing a separate for-loop
        page->y = y;
        widestPageWidth = std::max(widestPageWidth, page->width);
        totalHeight += page->height;
        y += page->height + PAGE_MARGIN;
    }

    // Now that we know the width of the widest page, we can offset
    // smaller pages to center them horizontally
    for (int i = 0; i < pageCount; i++) {
        Page *page = pages[i];
        page->x = std::max(0, (widestPageWidth - page->width) / 2);
    }

    // Shrink this widget to fit its contents exactly, and let the parent
    // QScrollArea worry about centering it in the viewport
    resize(widestPageWidth, totalHeight);
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
    setPagePixmap(num, QPixmap::fromImage(image));
}

void PagedContent::setPagePixmap(int num, const QPixmap &pixmap)
{
    if (0 <= num && num < pages.count()) {
        Page *page = pages[num];
        page->isRendering = false;
        page->pixmap = pixmap;
        // Paint at the correct physical size on high-DPI screens
        page->pixmap.setDevicePixelRatio(devicePixelRatio());
        update(page->rect());
    }
}

/*
 * Show the next frame of an animation.
 */
void PagedContent::showNextFrame(const QRect &rect)
{
    // Animations are always effectively single-"page" documents
    if (movie != nullptr)
        setPagePixmap(0, movie->currentPixmap());
}
