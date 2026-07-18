/*
 * Widget for viewing paged content like a PDF document.
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

#ifndef VIEWER_PAGED_H
#define VIEWER_PAGED_H

#include <QObject>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QImage>
#include <QMovie>
#include <QTimer>

#include <QWidget>
#include <QScrollArea>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QWheelEvent>

class PagedContent; // defined below
struct Page;        // defined in viewer_paged.cpp

class Renderer;
class PagedContentRenderer;

class PagedContentViewer : public QScrollArea
{
    Q_OBJECT

public:
    PagedContentViewer(QWidget *parent);

    QSize sizeHint() const;

    void setRenderer(Renderer *replacement);
    void setZoomFactor(int percent);

    QPoint scrollBarPosition() const;
    void setScrollBarPosition(int x, int y);
    inline void setScrollBarPosition(const QPoint &point)
        { setScrollBarPosition(point.x(), point.y()); }

public slots:
    void clear();
    void display();
    void refresh();

private:
    void wheelEvent(QWheelEvent *event);

    PagedContent *content;

signals:
    void wheelZoomed(int delta);
};

/*
 * This class is considered an implementation detail.
 * All interactions should go through PagedContentViewer.
 */
class PagedContent : public QWidget
{
    Q_OBJECT

protected:
    PagedContent(QScrollArea *parent);
    ~PagedContent();
    void setRenderer(Renderer *replacement);
    void setZoomFactor(int percent);

    friend class PagedContentViewer;

protected slots:
    void clear();
    void display();
    void refresh();

private:
    // Qt events
    void moveEvent(QMoveEvent *event);
    void paintEvent(QPaintEvent *event);

    // Other private methods
    void checkVisiblePages();
    void purgeCache();
    void updatePageGeometry();

    PagedContentRenderer *renderer;
    QList<Page*> pages;
    // We use a list rather than a queue for this because Qt may generate
    // multiple paint events between refresh()es
    QList<int> visiblePages;
    QMovie *movie;
    QTimer *renderTimer;
    int zoomFactor;

private slots:
    void renderVisiblePages();
    void setPageImage(int num, const QImage &image);
    void showNextFrame(const QRect &rect);

signals:
    void imageRequested(int num);
};

#endif /* VIEWER_PAGED_H */
