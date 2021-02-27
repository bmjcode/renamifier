/*
 * Support for reading various file formats.
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

#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>  // inherited by basically everything else
#include <QString>
#include <QMimeType>

/*
 * The Renderer processes files into a format the Viewer can display.
 *
 * It is intended to run in its own QThread, to prevent long render operations
 * from locking up the user interface.
 *
 * If you're adding support for a new file format, this is the place to start.
 */
class Renderer : public QObject
{
    Q_OBJECT

public:
    Renderer(const QString &path, int dpiX, int dpiY);

    static void init();

    enum RenderMode { TextContent, PagedContent };

public slots:
    void render();

signals:
    void renderMode(int mode);  // should be a value from RenderMode
    void renderProgress(int pagesDone, int pagesTotal);
    // Use for standard image files
    void renderedImage(const QImage &image);
    // Use for pages from a multi-page document like a PDF file
    void renderedPage(const QImage &image);
    // Use for plain text
    void renderedText(const QString &text);

private:
    QString path;
    QMimeType mimeType;
    int dpiX, dpiY;

    void renderError(const QString &details = QString());
    void renderImage();
    void renderPDF(void *document = nullptr);
    void renderPS();
    void renderXPS();
    void renderText();
    void renderTIFF();
    void renderUnidentified();
};

#endif /* RENDERER_H */
