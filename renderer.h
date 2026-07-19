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
    static Renderer *create(const QString &path,
                            QString *errorOut = nullptr);
    static void init();

    inline QString path() const { return m_path; }

    enum Mode { TextContent, PagedContent };
    virtual Renderer::Mode mode() const = 0;

protected:
    Renderer();
    inline void setPath(const QString &path) { m_path = path; }

    // This runs in the constructor to load the file specified by path().
    // Override this and return true if the file loaded, false otherwise.
    virtual bool load() = 0;

    // load() runs in the constructor so it can't use signals for this
    static void storeLoadError(const QString &message);

private:
    QString m_path;

signals:
    void errorEncountered(const QString &details = QString());
};

/*
 * Base class for text content renderers.
 *
 * render() is called once when the file is initially displayed.
 * It passes back the entire file contents via the renderedText signal.
 */
class TextContentRenderer : public Renderer
{
    Q_OBJECT

public:
    inline Renderer::Mode mode() const { return TextContent; }

public slots:
    virtual void render() = 0;

protected:
    TextContentRenderer();

signals:
    void renderedText(const QString &text);
};

/*
 * Base class for paged content renderers.
 *
 * renderPage() is called through a signal from the viewer. It passes back
 * a QImage of the requested page's contents via the renderedPage signal.
 *
 * Your subclass should also implement numPages(), which returns the total
 * number of pages in the file, and pageSize(), which returns the dimensions
 * in pixels of the specified page.
 */
class PagedContentRenderer : public Renderer
{
    Q_OBJECT

public:
    virtual int numPages() const = 0;
    virtual QSize pageSize(int num) const = 0;  // in pixels

    inline Renderer::Mode mode() const { return PagedContent; }

    // This hints to the viewer to determine how to handle high-DPI scaling.
    // If this is true, 1 screen pixel == 1 image pixel regardless of DPI;
    // otherwise, scaling is based on the image's inch, not pixel, dimensions.
    // Note this is just a hint, and the renderer itself is DPI-agnostic.
    virtual inline bool isPixelExact() const { return false; }

    // If this is true, the viewer will paint placeholders for any pages
    // that are still rendering.
    virtual inline bool shouldPaintPlaceholders() const { return true; }

    inline int dpiX() const { return m_dpiX; }
    inline int dpiY() const { return m_dpiY; }
    void setPixelDensity(int dpiX, int dpiY);

    inline int zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(int percent);

    inline bool pageExists(int num) const
        { return (0 <= num && num < numPages()); }

public slots:
    virtual void renderPage(int num) = 0;

protected:
    PagedContentRenderer();

    inline int zoomScaled(int value) const
        { return (m_zoomFactor == 100) ? value : value * m_zoomFactor / 100; }
    inline QSize zoomScaled(const QSize &size) const
        { return (m_zoomFactor == 100) ? size : size * m_zoomFactor / 100; }

private:
    int m_dpiX, m_dpiY;
    int m_zoomFactor;

signals:
    void renderedPage(int num, const QImage &image);
};

#endif /* RENDERER_H */
