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
    renderer->moveToThread(renderThread);

    // Signals common to all Renderers
    connect(renderer, &Renderer::renderedText, this, &Viewer::setText);
    connect(renderer, &Renderer::modeChanged, this, &Viewer::setMode);

    // Signals specific to PagedContentRenderer
    if (renderer->mode() == Renderer::PagedContent) {
        PagedContentRenderer *pcRenderer = (PagedContentRenderer*)renderer;

        connect(pagedContent, &PagedContent::pageRequested,
                pcRenderer, &PagedContentRenderer::renderPage);
        connect(pcRenderer, &PagedContentRenderer::renderedPage,
                this, &Viewer::setPageImage);
    }

    setMode(renderer->mode());
    startRender();
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
    if (currentWidget() == pagedContentViewer) {
        // Preserve the current scrollbar position
        QScrollBar *hScrollBar = pagedContentViewer->horizontalScrollBar(),
            *vScrollBar = pagedContentViewer->verticalScrollBar();
        int xPos, yPos;

        if (hScrollBar != nullptr)
            xPos = hScrollBar->sliderPosition();
        if (vScrollBar != nullptr)
            yPos = vScrollBar->sliderPosition();

        clear();
        repaginate();   // automatically triggers pagedContent->update()

        if (hScrollBar != nullptr)
            hScrollBar->setSliderPosition(xPos);
        if (vScrollBar != nullptr)
            vScrollBar->setSliderPosition(yPos);
    }
}

void Viewer::setMode(Renderer::Mode mode)
{
    switch (mode) {
    case Renderer::TextContent:
        setCurrentWidget(textContentViewer);
        break;

    case Renderer::PagedContent:
        setCurrentWidget(pagedContentViewer);
        break;
    }
}

void Viewer::setPageImage(int num, const QImage &image)
{
    pagedContent->setPageImage(num, image);
    pagedContent->update();
}

void Viewer::setText(const QString &text)
{
    textContentViewer->setPlainText(text);
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

void Viewer::repaginate()
{
    int pageCount;

    if (renderer == nullptr || renderer->mode() != Renderer::PagedContent)
        return;

    PagedContentRenderer *pcRenderer = (PagedContentRenderer*)renderer;

    pcRenderer->setZoomFactor(zoomFactor);
    // Always use logical DPI for correctly-scaled output on high-DPI screens
    pcRenderer->setPixelDensity(logicalDpiX(), logicalDpiY());

    pageCount = pcRenderer->numPages();
    pagedContent->reservePages(pageCount);

    for (int i = 0; i < pageCount; i++)
        pagedContent->setPageSize(i, pcRenderer->pageSize(i));
    pagedContent->recalculateArea();
}

void Viewer::startRender()
{
    if (renderer == nullptr)
        return;

    switch (renderer->mode()) {
    case Renderer::TextContent:
        QTimer::singleShot(
            0, (TextContentRenderer*)renderer, &TextContentRenderer::render);
        break;

    case Renderer::PagedContent:
        repaginate();
        break;
    }
}
