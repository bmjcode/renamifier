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
#include <QVector>

#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "renderer.h"

// For internal use
class TextContentViewer;
class PagedContentViewer;

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

    void display(const QString &path);
    void setFocusPolicy(Qt::FocusPolicy policy);

    QSize sizeHint() const;

public slots:
    void clear();
    void stopRender();

protected:
    Renderer *renderer;
    QThread *renderThread;
    // The Viewer widget automatically selects the most appropriate
    // of these to display its content
    TextContentViewer *textContentViewer;
    PagedContentViewer *pagedContentViewer;

protected slots:
    void addImage(const QImage &image);
    void addPage(const QImage &image);
    void addText(const QString &text);
    void setRenderMode(int mode);

signals:
    void renderProgress(int pagesDone, int pagesTotal);
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
    ~PagedContentViewer();

    void addImage(const QImage &image);
    void addPage(const QImage &image);
    void addText(const QString &text);
    void clear();

    QSize sizeHint() const;

private:
    QFrame *frame;
    QVBoxLayout *layout;
    QVector<QLabel*> pageWidgets;
    QVector<QLabel*> textWidgets;
    int totalPageWidth, totalPageHeight;

    QLabel* createContentWidget(bool drawBorder = true);
    void addPage_(const QImage &image, bool drawBorder = true);
    void resizeEvent(QResizeEvent *event);

private slots:
    void resizeFrame();
};

#endif /* VIEWER_H */
