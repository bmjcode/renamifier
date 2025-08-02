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

#include <QtCore>

#include "renderer.h"

/*
 * A simple image renderer.
 */
class ImageRenderer : public Renderer {
    Q_OBJECT

public:
    ImageRenderer(const QString &path);

    void render();
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
    PDFRenderer(const QString &path);

    static void init();

    void render();

private:
    QByteArray convertFromPostscript();
    QByteArray convertFromXPS();
};

/*
 * Renderer for plain text documents.
 */
class TextRenderer : public Renderer {
    Q_OBJECT

public:
    TextRenderer(const QString &path);

    void render();
};

/*
 * Fallback renderer for unknown file formats.
 */
class UnknownFormatRenderer : public Renderer {
    Q_OBJECT

public:
    UnknownFormatRenderer(const QString &path);

    void render();
};

#endif /* RENDER_FORMATS_H */
