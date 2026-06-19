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

#include <QtCore>

#include "render_pdf.h"
#include "renderer_util.h"

static QString popplerError;

static void storePopplerError(const QString &message, const QVariant &closure);

PDFRenderer::PDFRenderer()
    : Renderer()
{
    document = nullptr;
    popplerError.clear();
}

void PDFRenderer::load()
{
    document = Poppler::Document::load(path_);
}

void PDFRenderer::init()
{
    Poppler::setDebugErrorFunction(&storePopplerError, QVariant());
}

QSize PDFRenderer::pageSize(int num) const
{
    if (document != nullptr) {
        std::unique_ptr<Poppler::Page> page = document->page(num);
        if (page != nullptr) {
            QSize pointSize = page->pageSize();
            // Convert points to pixels at our current DPI
            return zoomScaled(QSize(pointSize.width() * dpiX_ / 72,
                                    pointSize.height() * dpiY_ / 72));
        }
    }
    return QSize(0, 0);
}

void PDFRenderer::renderPage(int num)
{
    int xRes = zoomScaled(dpiX_), yRes = zoomScaled(dpiY_);

    // Check whether an error occurred while rendering this document
    if (document == nullptr) {
        renderError(popplerError);
        return;
    }

    // Make the document look nice on screen
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    emit renderMode(PagedContent);
    emit renderedPage(num, document->page(num)->renderToImage(xRes, yRes));
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
