/*
 * Base class for file format renderers.
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

#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>  // inherited by basically everything else
#include <QByteArray>
#include <QSize>
#include <QString>
#include <QImage>

/*
 * Base class for all Renderers.
 *
 * A Renderer processes files into a format the Viewer can display. It is
 * intended to run in a separate QThread to prevent long render operations
 * from locking up the user interface.
 *
 * This class defines the basic renderer API but does not itself implement
 * any rendering logic. See render_*.h and render_*.cpp for that.
 *
 * Use Renderer::create() to create a renderer for a given file. This will
 * automatically select the correct subclass to use based on the file type.
 *
 * Renderer implementations should inherit one of the specific subtypes
 * TextContentRenderer or PagedContentRenderer, which provide additional
 * common methods, signals, and slots specific to their rendering needs.
 */
class Renderer : public QObject
{
    Q_OBJECT

public:
    static Renderer *create(const QString &path);
    static void init();

    inline QString path() const { return path_; }

    enum Mode { TextContent, PagedContent };
    virtual Renderer::Mode mode() const = 0;

signals:
    void errorEncountered(const QString &details);

protected:
    QString path_;

    Renderer();
    virtual void load() = 0;

    void renderError(const QString &details = QString());
    QByteArray runHelper(const QString &program,
                         const QStringList &arguments);
};

/*
 * Base class for text content renderers.
 */
class TextContentRenderer : public Renderer {
    Q_OBJECT

public slots:
    virtual void render() = 0;

signals:
    void renderedText(const QString &text);

protected:
    TextContentRenderer();
    inline Renderer::Mode mode() const { return TextContent; }
};

/*
 * Base class for paged content renderers.
 */
class PagedContentRenderer : public Renderer {
    Q_OBJECT

public:
    virtual int numPages() const = 0;
    virtual QSize pageSize(int num) const = 0;  // in pixels

    inline int zoomFactor() const { return zoomFactor_; }

    inline void setPixelDensity(int dpiX, int dpiY)
        { dpiX_ = dpiX, dpiY_ = dpiY; }
    inline void setZoomFactor(int percent) { zoomFactor_ = percent; }

public slots:
    virtual void renderPage(int num) = 0;

signals:
    void renderedPage(int num, const QImage &image);

protected:
    PagedContentRenderer();
    inline Renderer::Mode mode() const { return PagedContent; }

    inline int zoomScaled(int value) const
        { return (zoomFactor_ == 100) ? value : value * zoomFactor_ / 100; }
    inline QSize zoomScaled(const QSize &size) const
        { return (zoomFactor_ == 100) ? size : size * zoomFactor_ / 100; }

    int dpiX_, dpiY_;
    int zoomFactor_;
};

#endif /* RENDERER_H */
