/*
 * A widget to display file previews.
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

#ifndef VIEWER_H
#define VIEWER_H

#include <QObject>
#include <QList>
#include <QSize>
#include <QThread>

#include <QStackedWidget>

#include "renderer.h"

// We include the actual headers in viewer.cpp to limit the number of files
// that need recompiling when their internals change
class TextContentViewer;
class PagedContentViewer;
class PagedContent;

/*
 * File preview widget.
 *
 * Internally, the Viewer class does two things:
 *  1. Create a Renderer to read and process the file.
 *  2. Provide an appropriate viewer widget to display its content.
 *
 * In other words, it does not actually process or display content itself,
 * but rather manages and connects the other individual classes that do.
 */
class Viewer : public QStackedWidget
{
    Q_OBJECT

public:
    Viewer(QWidget *parent);
    ~Viewer();

    void display(const QString &path);
    void setFocusPolicy(Qt::FocusPolicy policy);

    QSize sizeHint() const;

public slots:
    void clear();
    void stopRender();

protected:
    QThread *renderThread;
    Renderer *renderer;
    // The Viewer widget automatically selects the most appropriate
    // of these to display its content
    TextContentViewer *textContentViewer;
    PagedContentViewer *pagedContentViewer;
    PagedContent *pagedContent;

protected slots:
    void addPage(int num, const QImage &image);
    void addText(const QString &text);
    void setRenderMode(int mode);
};

#endif /* VIEWER_H */
