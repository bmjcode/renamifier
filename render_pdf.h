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

#ifndef RENDER_PDF_H
#define RENDER_PDF_H

#include <memory>   // for std::unique_ptr

#include <QObject>
#include <QSize>
#include <QByteArray>

#include <poppler-qt6.h>

#include "renderer.h"

class PDFRenderer : public Renderer {
    Q_OBJECT

public:
    static void init();

    PDFRenderer();
    virtual void load();
    void renderPage(int num);

    inline int numPages() const
        { return (document == nullptr) ? 0 : document->numPages(); }
    QSize pageSize(int num) const;

protected:
    std::unique_ptr<Poppler::Document> document;
};

#endif /* RENDER_PDF_H */
