/*
 * A widget to display file previews.
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

#include <algorithm>    // for std::clamp()

#include <QtCore>
#include <QtWidgets>

#include "viewer.h"
#include "viewer_text.h"
#include "viewer_paged.h"
#include "renderer.h"

/* ------------------------------------------------------------------------ */

#define ZOOM_MIN 10
#define ZOOM_MAX 800

// The initial viewer size is 8.5 x 5.5 in, or half of a US letter page.
// This fits a reasonable amount of content without making drastic assumptions
// about the size of the user's screen, and approximates the 16:9 or 16:10
// aspect ratio found on most modern displays.
#define INITIAL_WIDTH 85
#define INITIAL_HEIGHT 55
// Units above are multiplied by a factor of 10 to allow use of integer math.
#define INITIAL_FACTOR 10

/* ------------------------------------------------------------------------ */

Viewer::Viewer(QWidget *parent)
    : QStackedWidget(parent)
{
    textContentViewer = new TextContentViewer(this);
    addWidget(textContentViewer);

    pagedContentScrollArea = new ViewerScrollArea(this);
    addWidget(pagedContentScrollArea);

    pagedContent = new PagedContent(pagedContentScrollArea);
    pagedContentScrollArea->setWidget(pagedContent);

    renderer = nullptr;
    renderThread = new QThread(this);
    renderThread->start();

    zoomFactor = 100;
    connect(textContentViewer, &TextContentViewer::zoomChanged,
            this, &Viewer::setZoom);
    connect(pagedContentScrollArea, &ViewerScrollArea::zoomChanged,
            this, &Viewer::setZoom);
}

Viewer::~Viewer()
{
    unloadRenderer();
    if (renderThread != nullptr) {
        renderThread->quit();
        renderThread->wait();
    }
}

/*
 * Load and display the specified file.
 */
void Viewer::display(const QString &path)
{
    clear();
    load(path);
    refresh();
}

/*
 * Create and connect a renderer for the specified file,
 * but do not immediately display it.
 */
void Viewer::load(const QString &path)
{
    QString loadError;
    unloadRenderer();

    path_ = path;
    renderer = Renderer::create(path_, &loadError);
    if (renderer == nullptr) {
        // An error occurred while loading the file
        displayError(loadError);
        return;
    }

    renderer->moveToThread(renderThread);

    connect(renderer, &Renderer::errorEncountered,
            this, &Viewer::displayError);

    // These will reject one another's Renderers, so no need to overthink this
    textContentViewer->setRenderer(renderer);
    pagedContent->setRenderer(renderer);
}

/*
 * Disconnect and delete the current renderer.
 */
void Viewer::unloadRenderer()
{
    path_.clear();
    textContentViewer->setRenderer(nullptr);
    pagedContent->setRenderer(nullptr);

    if (renderer != nullptr) {
        // Don't respond to any more signals from this Renderer
        disconnect(renderer, nullptr, nullptr, nullptr);
        // Qt gets upset and segfaults if we delete this directly
        renderer->deleteLater();
        renderer = nullptr;
    }
}

void Viewer::setFocusPolicy(Qt::FocusPolicy policy)
{
    textContentViewer->setFocusPolicy(policy);
    pagedContentScrollArea->setFocusPolicy(policy);
}

/*
 * Clear displayed content.
 */
void Viewer::clear()
{
    // Do NOT unload the renderer here; we may want to reuse it
    textContentViewer->clear();
    pagedContent->clear();

    // Scroll back to the top-left corner
    pagedContentScrollArea->setScrollBarPosition(0, 0);
}

/*
 * Update the display.
 */
void Viewer::refresh()
{
    if (renderer == nullptr)
        return;

    // Do not clear() here! The entire _point_ is that we do not clear() here.
    // (We don't want its side effects like changing the scrollbar position)
    switch (renderer->mode()) {
    case Renderer::TextContent:
        setCurrentWidget(textContentViewer);
        textContentViewer->display();
        break;
    case Renderer::PagedContent:
        setCurrentWidget(pagedContentScrollArea);
        pagedContent->display();
        break;
    }
}

void Viewer::setZoom(int percent)
{
    zoomFactor = std::clamp(percent, ZOOM_MIN, ZOOM_MAX);
    textContentViewer->setZoomFactor(zoomFactor);
    pagedContent->setZoomFactor(zoomFactor);

    if (currentWidget() == pagedContentScrollArea) {
        QPoint where = pagedContentScrollArea->scrollBarPosition();
        pagedContent->refresh();
        pagedContentScrollArea->setScrollBarPosition(where);
    }
}

void Viewer::displayError(const QString &details)
{
    QString message;
    QTextStream textStream(&message);
    QString path = path_;   // save this before unloadRenderer() clears it

    // Stop rendering immediately; we'll do other cleanup later
    unloadRenderer();

    // Tell the user what happened
    textStream << "An error occurred while attempting to display this file:"
               << Qt::endl
               << path;

    // Append details if we have them
    if (!details.isEmpty())
        textStream << Qt::endl
                   << Qt::endl
                   << details;

    clear();
    setCurrentWidget(textContentViewer);
    textContentViewer->setPlainText(message);
}

/*
 * Default to the paged content viewer's preferred size.
 */
QSize Viewer::sizeHint() const
{
    return pagedContentScrollArea->sizeHint();
}

/* ------------------------------------------------------------------------ */

ViewerScrollArea::ViewerScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    setBackgroundRole(QPalette::Dark);
}

QPoint ViewerScrollArea::scrollBarPosition() const {
    return QPoint(
        horizontalScrollBar()->sliderPosition(),
        verticalScrollBar()->sliderPosition());
}

void ViewerScrollArea::setScrollBarPosition(int x, int y)
{
    horizontalScrollBar()->setSliderPosition(x);
    verticalScrollBar()->setSliderPosition(y);
}

/*
 * Default to a size large enough to show a reasonable amount of content on
 * most screens. The exact size is specified by INITIAL_{HEIGHT,WIDTH} above.
 */
QSize ViewerScrollArea::sizeHint() const
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
void ViewerScrollArea::resizeEvent(QResizeEvent *event)
{
    widget()->resize(
        std::max(viewport()->width(), widget()->minimumWidth()),
        std::max(viewport()->height(), widget()->minimumHeight()));
}
