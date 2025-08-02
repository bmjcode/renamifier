/*
 * Interface for file format renderers.
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

#include "renderer.h"
#include "render_formats.h"

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
        renderer = new PDFRenderer(path);

    // More generic MIME types
    // These come last since more specific types may inherit from them
    else if (mimeType.name().startsWith("image/"))
        renderer = new ImageRenderer(path);
    else if (mimeType.inherits("text/plain"))
        renderer = new TextRenderer(path);

    // Fallback if we can't identify this file
    else
        renderer = new UnknownFormatRenderer(path);

    renderer->setDPI(dpiX, dpiY);
    renderer->setMimeType(mimeType);
    return renderer;
}

/*
 * Initialize the renderer.
 */
void Renderer::init()
{
    PDFRenderer::init();
}

/*
 * Construct a new Renderer.
 */
Renderer::Renderer(const QString &path_)
{
    path = path_;
    dpiX = 0;
    dpiY = 0;
}

/*
 * Display an error if we were unable to render a file.
 */
void Renderer::renderError(const QString &details)
{
    QString message;
    QTextStream textStream(&message);

    // Tell the user what happened
    textStream << "An error occurred while attempting to display this file:"
               << Qt::endl
               << path;

    // Append details if we have them
    if (!details.isEmpty())
        textStream << Qt::endl
                   << Qt::endl
                   << details;

    emit renderMode(TextContent);
    emit renderedText(message);
}
