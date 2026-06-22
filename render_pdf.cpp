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
static QMutex popplerErrorMutex;

static void storePopplerError(const QString &message, const QVariant &closure);

void PDFRenderer::init()
{
    Poppler::setDebugErrorFunction(&storePopplerError, QVariant());
}

PDFRenderer::PDFRenderer()
    : PagedContentRenderer()
{
    data = new PDFRendererData;
    data->document = nullptr;
}

PDFRenderer::~PDFRenderer()
{
    delete data;
}

bool PDFRenderer::load()
{
    QMutexLocker locker(&popplerErrorMutex);
    popplerError.clear();

    data->document = Poppler::Document::load(path());
    if (data->document == nullptr) {
        setLoadError(popplerError);
        return false;
    } else
        return true;
}

void PDFRenderer::renderPage(int num)
{
    QMutexLocker locker(&popplerErrorMutex);
    popplerError.clear();

    if (data->document == nullptr)
        return; // usually the renderer would be deleted before we got here

    // Make the document look nice on screen
    data->document->setRenderHint(Poppler::Document::Antialiasing);
    data->document->setRenderHint(Poppler::Document::TextAntialiasing);

    int xRes = zoomScaled(dpiX_), yRes = zoomScaled(dpiY_);
    QImage image = data->document->page(num)->renderToImage(xRes, yRes);
    if (image.isNull())
        emit errorEncountered(popplerError);
    else
        emit renderedPage(num, image);
}

int PDFRenderer::numPages() const
{
    return (data->document == nullptr) ? 0 : data->document->numPages();
}

// Note this is not const because emitting a signal changes internal state
QSize PDFRenderer::pageSize(int num)
{
    QMutexLocker locker(&popplerErrorMutex);
    popplerError.clear();

    if (data->document != nullptr) {
        std::unique_ptr<Poppler::Page> page = data->document->page(num);
        if (page == nullptr)
            emit errorEncountered(popplerError);
        else {
            QSize pointSize = page->pageSize();
            // Convert points to pixels at our current DPI
            return zoomScaled(QSize(pointSize.width() * dpiX_ / 72,
                                    pointSize.height() * dpiY_ / 72));
        }
    }
    return QSize(0, 0);
}

bool PDFRenderer::loadFromData(const QByteArray &bytes)
{
    QMutexLocker locker(&popplerErrorMutex);
    popplerError.clear();

    data->document = Poppler::Document::loadFromData(bytes);
    if (data->document == nullptr) {
        // If `bytes` came from a helper program, this now contains its output
        QString message = loadError();
        // Append any additional error messages from Poppler
        if (!message.isEmpty())
            message += "\n";
        message += popplerError;
        setLoadError(message);
        return false;
    } else
        return true;
}

/*
 * Stores debug and error messages from Poppler so we can display them
 * in the application.
 * Note the mutex has already been locked by the time we get here.
 */
void storePopplerError(const QString &message, const QVariant &closure)
{
    (void)closure;
    // Do not clear popplerError here; any given Poppler method call may
    // result in more than one error, and we want to preserve them all
    if (!popplerError.isEmpty())
        popplerError += "\n";
    popplerError += message;
}
