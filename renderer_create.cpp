/*
 * Interface for file format renderers.
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

#include <QMimeType>
#include <QMimeDatabase>

#include "renderer.h"

// Available renderers
// List alphabetically by name
#include "render_hexdump.h"
#include "render_image.h"
#include "render_pdf.h"
#include "render_text.h"

/*
 * Return an appropriate renderer subclass for the specified path.
 */
Renderer *Renderer::create(const QString &path, int dpiX, int dpiY)
{
    Renderer *renderer;
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(path);

    // Specific MIME types
    // List alphabetically by name
    if (mimeType.inherits("application/oxps")
        || mimeType.inherits("application/pdf")
        || mimeType.inherits("application/postscript")
        || mimeType.inherits("application/xps"))
        renderer = new PDFRenderer;

    // More generic MIME types
    // These come last since more specific types may inherit from them
    else if (mimeType.name().startsWith("image/"))
        renderer = new ImageRenderer;
    else if (mimeType.inherits("text/plain"))
        renderer = new TextRenderer;

    // Fallback if we can't identify this file
    else
        renderer = new HexDumpRenderer;

    renderer->path_ = path;
    renderer->mimeType_ = mimeType;
    renderer->dpiX_ = dpiX;
    renderer->dpiY_ = dpiY;
    renderer->load();
    return renderer;
}

/*
 * Initialize the renderer.
 */
void Renderer::init()
{
    PDFRenderer::init();
}
