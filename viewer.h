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

#include <QObject>  // inherited by basically everything else
#include <QThread>
#include <QList>
#include <QSize>

#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPaintEvent>

#include "renderer.h"

// For internal use
class TextContentViewer;
class PagedContentViewer;
class PagedContent;
struct Page;

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

/*
 * Widget for viewing plain text content.
 */
class TextContentViewer : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextContentViewer(QWidget *parent);
};

/*
 * Widget for viewing paged content like a PDF document.
 *
 * This can also display plain text content, but TextContentViewer
 * provides more features and is vastly more efficient.
 */
class PagedContentViewer : public QScrollArea
{
    Q_OBJECT

public:
    PagedContentViewer(QWidget *parent);

    QSize sizeHint() const;

private:
    void resizeEvent(QResizeEvent *event);
};

class PagedContent : public QFrame
{
    Q_OBJECT

public:
    PagedContent(QWidget *parent);
    ~PagedContent();

    void clear();
    void reservePages(int numPages);
    void setContentSize(int w, int h);
    void setPageImage(int num, const QImage &image);
    void setPageSize(int num, int w, int h);

private:
    void paintEvent(QPaintEvent *event);

    QList<Page*> pages;
    QSize contentSize_;

signals:
    void pageRequested(int num);
};

#endif /* VIEWER_H */
