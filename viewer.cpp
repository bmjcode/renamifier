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

#define ZOOM_MIN 10
#define ZOOM_MAX 800

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

    zoomFactor = 100;
    connect(textContentViewer, &TextContentViewer::zoomChanged,
            this, &Viewer::setZoom);
    connect(pagedContentViewer, &PagedContentViewer::zoomChanged,
            this, &Viewer::setZoom);

    connect(this, &QStackedWidget::currentChanged,
            this, &Viewer::handleCurrentChanged);
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
    unloadRenderer();

    path_ = path;
    renderer = Renderer::create(path_);
    if (renderer == nullptr) {
        // This should never fail, but...
        displayError("Failed to create an appropriate renderer.");
        return;
    } else if (!renderer->isReady()) {
        // An error occurred while loading the file
        displayError(renderer->loadError());
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
        delete renderer;
        renderer = nullptr;
    }
}

void Viewer::setFocusPolicy(Qt::FocusPolicy policy)
{
    textContentViewer->setFocusPolicy(policy);
    pagedContentViewer->setFocusPolicy(policy);
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
    pagedContentViewer->horizontalScrollBar()->setSliderPosition(0);
    pagedContentViewer->verticalScrollBar()->setSliderPosition(0);
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
        setCurrentWidget(pagedContentViewer);
        pagedContent->display();
        break;
    }
}

void Viewer::setZoom(int percent)
{
    zoomFactor = std::clamp(percent, ZOOM_MIN, ZOOM_MAX);
    textContentViewer->setZoomFactor(zoomFactor);
    pagedContent->setZoomFactor(zoomFactor);

    if (currentWidget() == pagedContentViewer) {
        // Preserve the current scrollbar position
        QScrollBar *hScrollBar = pagedContentViewer->horizontalScrollBar(),
            *vScrollBar = pagedContentViewer->verticalScrollBar();
        int xPos, yPos;

        xPos = hScrollBar->sliderPosition();
        yPos = vScrollBar->sliderPosition();

        pagedContent->refresh();

        hScrollBar->setSliderPosition(xPos);
        vScrollBar->setSliderPosition(yPos);
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

void Viewer::handleCurrentChanged(int index)
{
    // Only update pagedContent when it is visible
    pagedContent->setUpdatesEnabled(currentWidget() == pagedContentViewer);
}

/*
 * Default to PagedContentViewer's preferred size.
 */
QSize Viewer::sizeHint() const
{
    return pagedContentViewer->sizeHint();
}
