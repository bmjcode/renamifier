/*
 * Support for reading various file formats.
 * Copyright (c) 2021, 2025 Benjamin Johnson
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

#ifndef RENDER_FORMATS_H
#define RENDER_FORMATS_H

#include <memory>

#include <QtCore>
#include <QImage>
#include <poppler-qt6.h>

#include "renderer.h"

/*
 * A simple image renderer.
 */
class ImageRenderer : public Renderer {
    Q_OBJECT

public:
    ImageRenderer();

    inline int numPages() const { return 1; }
    inline QSize pageSize(int num) const { return image.size(); }

    void load();
    void renderPage(int num);

private:
    QImage image;
};

/*
 * Renderer for PDF documents.
 *
 * Additional formats that can be converted to PDF are supported if their
 * respective helper programs are present:
 *
 *   - Postscript (requires Ghostscript)
 *   - XPS (requires GhostXPS)
 */
class PDFRenderer : public Renderer {
    Q_OBJECT

public:
    PDFRenderer();

    static void init();

    inline int numPages() const
        { return (document == nullptr) ? 0 : document->numPages(); }
    QSize pageSize(int num) const;

    void load();
    void renderPage(int num);

private:
    QByteArray convertFromPostscript();
    QByteArray convertFromXPS();

    std::unique_ptr<Poppler::Document> document;
};

/*
 * Renderer for plain text documents.
 */
class TextRenderer : public Renderer {
    Q_OBJECT

public:
    TextRenderer();

    inline int numPages() const { return 1; }
    inline QSize pageSize(int num) const { return QSize(0, 0); }

    void load() { }
    void renderPage(int num);
};

/*
 * Fallback renderer for unknown file formats.
 */
class UnknownFormatRenderer : public Renderer {
    Q_OBJECT

public:
    UnknownFormatRenderer();

    inline int numPages() const { return 1; }
    inline QSize pageSize(int num) const { return QSize(0, 0); }

    void load() { }
    void renderPage(int num);
};

#endif /* RENDER_FORMATS_H */
