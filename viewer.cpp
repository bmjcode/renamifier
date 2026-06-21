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

#include <QtCore>
#include <QtWidgets>

#include "viewer.h"
#include "viewer_text.h"
#include "viewer_paged.h"

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
    clear();
    deleteRenderer();
    if (renderThread != nullptr) {
        renderThread->quit();
        renderThread->wait();
    }
}

void Viewer::display(const QString &path)
{
    clear();
    deleteRenderer();

    renderer = Renderer::create(path);
    if (renderer == nullptr)
        return; // this should never fail, but...
    renderer->moveToThread(renderThread);

    connect(renderer, &Renderer::errorEncountered,
            this, &Viewer::displayError);

    Renderer::Mode mode = renderer->mode();
    setMode(mode);
    switch (mode) {
    case Renderer::TextContent:
        textContentViewer->setRenderer(renderer);
        break;
    case Renderer::PagedContent:
        pagedContent->setRenderer(renderer);
        pagedContent->refresh();
        break;
    }
}

void Viewer::setFocusPolicy(Qt::FocusPolicy policy)
{
    textContentViewer->setFocusPolicy(policy);
    pagedContentViewer->setFocusPolicy(policy);
}

void Viewer::clear()
{
    // Do NOT delete the renderer here; we may want to reuse it
    stopRender();
    textContentViewer->clear();
    pagedContent->clear();
}

void Viewer::stopRender()
{
    // This currently does nothing, but is kept just in case we need it again
}

void Viewer::setZoom(int percent)
{
    zoomFactor = percent;
    textContentViewer->setZoomFactor(zoomFactor);
    pagedContent->setZoomFactor(zoomFactor);

    if (currentWidget() == pagedContentViewer) {
        // Preserve the current scrollbar position
        QScrollBar *hScrollBar = pagedContentViewer->horizontalScrollBar(),
            *vScrollBar = pagedContentViewer->verticalScrollBar();
        int xPos, yPos;

        if (hScrollBar != nullptr)
            xPos = hScrollBar->sliderPosition();
        if (vScrollBar != nullptr)
            yPos = vScrollBar->sliderPosition();

        pagedContent->refresh();

        if (hScrollBar != nullptr)
            hScrollBar->setSliderPosition(xPos);
        if (vScrollBar != nullptr)
            vScrollBar->setSliderPosition(yPos);
    }
}

void Viewer::displayError(const QString &details)
{
    clear();
    deleteRenderer();

    setCurrentWidget(textContentViewer);
    textContentViewer->setPlainText(details);
}

void Viewer::handleCurrentChanged(int index)
{
    // Only update pagedContent when it is visible
    pagedContent->setUpdatesEnabled(currentWidget() == pagedContentViewer);
}

void Viewer::setMode(Renderer::Mode mode)
{
    switch (mode) {
    case Renderer::TextContent:
        setCurrentWidget(textContentViewer);
        break;
    case Renderer::PagedContent:
        setCurrentWidget(pagedContentViewer);
        pagedContent->refresh();
        break;
    }
}

/*
 * Default to PagedContentViewer's preferred size.
 */
QSize Viewer::sizeHint() const
{
    return pagedContentViewer->sizeHint();
}

void Viewer::deleteRenderer()
{
    stopRender();
    if (renderer != nullptr) {
        // Don't respond to any more signals from this Renderer
        disconnect(renderer, nullptr, nullptr, nullptr);
        delete renderer;
        renderer = nullptr;
    }
}
