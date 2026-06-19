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
    deleteRenderer();
    clear();
    if (renderThread != nullptr) {
        renderThread->quit();
        renderThread->wait();
    }
}

void Viewer::display(const QString &path)
{
    deleteRenderer();
    renderer = Renderer::create(path);
    renderer->moveToThread(renderThread);
    redisplay();
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
    if (currentWidget() == pagedContentViewer)
        redisplay();
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

void Viewer::redisplay()
{
    int pageCount;

    clear();
    renderer->setZoomFactor(zoomFactor);
    // Always use logical DPI for correctly-scaled output on high-DPI screens
    renderer->setPixelDensity(logicalDpiX(), logicalDpiY());

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
