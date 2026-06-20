/*
 * Renderer for PDF documents.
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

#include <memory>   // for std::unique_ptr

#include <QtCore>

#include <poppler-qt6.h>

#include "render_pdf.h"
#include "renderer_util.h"

struct PDFRendererData {
    std::unique_ptr<Poppler::Document> document;
};

static QString popplerError;

static void storePopplerError(const QString &message, const QVariant &closure);

void PDFRenderer::init()
{
    Poppler::setDebugErrorFunction(&storePopplerError, QVariant());
}

PDFRenderer::PDFRenderer()
    : Renderer()
{
    data = new PDFRendererData;
    data->document = nullptr;
    popplerError.clear();
}

PDFRenderer::~PDFRenderer()
{
    delete data;
}

void PDFRenderer::load()
{
    data->document = Poppler::Document::load(path_);
}

void PDFRenderer::renderPage(int num)
{
    // Check whether an error occurred while rendering this document
    if (data->document == nullptr) {
        renderError(popplerError);
        return;
    }

    // Make the document look nice on screen
    data->document->setRenderHint(Poppler::Document::Antialiasing);
    data->document->setRenderHint(Poppler::Document::TextAntialiasing);

    int xRes = zoomScaled(dpiX_), yRes = zoomScaled(dpiY_);
    emit renderedPage(num,
                      data->document->page(num)->renderToImage(xRes, yRes));
}

int PDFRenderer::numPages() const
{
    return (data->document == nullptr) ? 0 : data->document->numPages();
}

QSize PDFRenderer::pageSize(int num) const
{
    if (data->document != nullptr) {
        std::unique_ptr<Poppler::Page> page = data->document->page(num);
        if (page != nullptr) {
            QSize pointSize = page->pageSize();
            // Convert points to pixels at our current DPI
            return zoomScaled(QSize(pointSize.width() * dpiX_ / 72,
                                    pointSize.height() * dpiY_ / 72));
        }
    }
    return QSize(0, 0);
}

void PDFRenderer::loadFromData(const QByteArray &bytes)
{
    data->document = Poppler::Document::loadFromData(bytes);
}

/*
 * Stores debug and error messages from Poppler so we can display them
 * in the application.
 */
void storePopplerError(const QString &message, const QVariant &closure)
{
    (void)closure;
    if (!popplerError.isEmpty())
        popplerError += "\n";
    popplerError += message;
}
