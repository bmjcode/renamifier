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

    renderer = nullptr;
    renderThread = new QThread(this);
    renderThread->start();

    zoomFactor = 100;
    connect(textContentViewer, &TextContentViewer::wheelZoomed,
            this, &Viewer::zoomIn);
    connect(pagedContentViewer, &PagedContentViewer::wheelZoomed,
            this, &Viewer::zoomIn);
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
void Viewer::load(const QString &path_)
{
    QString loadError;
    unloadRenderer();

    path = path_;
    renderer = Renderer::create(path, &loadError);
    if (renderer == nullptr) {
        // An error occurred while loading the file
        displayError(loadError);
        return;
    }

    renderer->moveToThread(renderThread);

    // Error handling is standard across all renderers
    connect(renderer, &Renderer::errorEncountered,
            this, &Viewer::displayError);

    // We don't have to know which renderer goes with which subwidget here
    // because the subwidgets ignore renderers they don't recognize
    textContentViewer->setRenderer(renderer);
    pagedContentViewer->setRenderer(renderer);
}

/*
 * Disconnect and delete the current renderer.
 */
void Viewer::unloadRenderer()
{
    path.clear();
    textContentViewer->setRenderer(nullptr);
    pagedContentViewer->setRenderer(nullptr);

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
    pagedContentViewer->setFocusPolicy(policy);
}

/*
 * Clear displayed content.
 */
void Viewer::clear()
{
    // Do NOT unload the renderer here; we may want to reuse it
    textContentViewer->clear();
    pagedContentViewer->clear();
}

/*
 * Update the display.
 */
void Viewer::refresh()
{
    if (renderer == nullptr)
        return;

    // Note we don't clear() the subwidgets here because display() already
    // erases previous content, and we don't want side effects like changing
    // the scrollbar position
    switch (renderer->mode()) {
    case Renderer::TextContent:
        setCurrentWidget(textContentViewer);
        textContentViewer->display();
        break;
    case Renderer::PagedContent:
        setCurrentWidget(pagedContentViewer);
        pagedContentViewer->display();
        break;
    }
}

void Viewer::setFitToWidth(bool enabled)
{
    pagedContentViewer->setFitToWidth(enabled);
}

void Viewer::setZoom(int percent)
{
    // Using an if-statement rather than clamp() here avoids triggering a
    // re-display if the requested zoom level is outside our limits
    if (ZOOM_MIN <= percent && percent <= ZOOM_MAX) {
        zoomFactor = percent;
        textContentViewer->setZoomFactor(zoomFactor);
        pagedContentViewer->setZoomFactor(zoomFactor);
    }
}

void Viewer::displayError(const QString &details)
{
    QString message;
    QTextStream textStream(&message);
    textStream << "An error occurred while attempting to display this file:"
               << Qt::endl
               << path;
    if (!details.isEmpty())
        textStream << Qt::endl
                   << Qt::endl
                   << details;

    unloadRenderer();
    clear();
    setCurrentWidget(textContentViewer);
    textContentViewer->setPlainText(message);
}

/*
 * Default to the paged content viewer's preferred size.
 */
QSize Viewer::sizeHint() const
{
    return pagedContentViewer->sizeHint();
}
