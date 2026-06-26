/*
 * A widget to display file previews.
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

#ifndef VIEWER_H
#define VIEWER_H

#include <QObject>
#include <QList>
#include <QSize>
#include <QThread>

#include <QStackedWidget>

// We include the actual headers in viewer.cpp to limit the number of files
// that need recompiling when their internals change
class TextContentViewer;
class PagedContentViewer;
class PagedContent;
class Renderer;

/*
 * File preview widget.
 *
 * The Viewer presents a unified interface to several specialized widgets
 * that do the actual work of displaying the file.
 */
class Viewer : public QStackedWidget
{
    Q_OBJECT

public:
    Viewer(QWidget *parent);
    ~Viewer();

    void display(const QString &path);
    void load(const QString &path);
    void unloadRenderer();

    void setFocusPolicy(Qt::FocusPolicy policy);

    QSize sizeHint() const;

public slots:
    void clear();
    void refresh();

    void setZoom(int percent);
    inline void zoomActualSize()     { setZoom(100); }
    inline void zoomIn(int range=1)  { setZoom(zoomFactor + 10 * range); }
    inline void zoomOut(int range=1) { setZoom(zoomFactor - 10 * range); }

private:
    QThread *renderThread;
    // The Viewer class creates and owns the renderer, but the individual
    // widgets below handle most of the interaction with it
    Renderer *renderer;
    // Specialized widgets to display different types of content
    TextContentViewer *textContentViewer;
    PagedContentViewer *pagedContentViewer;
    PagedContent *pagedContent;
    QString path_;
    int zoomFactor;

private slots:
    void displayError(const QString &details);

signals:
    void zoomChanged(int percent);
};

#endif /* VIEWER_H */
