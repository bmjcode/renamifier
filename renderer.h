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

#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>  // inherited by basically everything else
#include <QByteArray>
#include <QString>
#include <QMimeType>

/*
 * A Renderer processes files into a format the Viewer can display.
 *
 * It is intended to run in its own QThread, to prevent long render operations
 * from locking up the user interface.
 *
 * This class defines the basic renderer API but does not itself implement
 * any rendering logic. See render_formats.h and render_formats.cpp for that.
 *
 * Use Renderer::create() to create a renderer for a given file. This will
 * automatically select the correct subclass to use based on the file type.
 */
class Renderer : public QObject
{
    Q_OBJECT

public:
    static Renderer *create(const QString &path, int dpiX, int dpiY);
    static void init();

    inline QString path() const { return path_; }
    inline QMimeType mimeType() const { return mimeType_; }
    inline int numPages() const { return numPages_; }

    enum RenderMode { TextContent, PagedContent };

public slots:
    virtual void render() = 0;

signals:
    void renderMode(int mode);  // should be a value from RenderMode
    void renderProgress(int pagesDone, int pagesTotal);
    // Use for standard image files
    void renderedImage(const QImage &image);
    // Use for pages from a multi-page document like a PDF file
    void renderedPage(const QImage &image);
    // Use for plain text
    void renderedText(const QString &text);

protected:
    QString path_;
    QMimeType mimeType_;
    int dpiX_, dpiY_, numPages_;

    Renderer();

    void renderError(const QString &details = QString());
    QByteArray runHelper(const QString &program,
                         const QStringList &arguments);
};

#endif /* RENDERER_H */
