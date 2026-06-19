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
#include <QSize>
#include <QImage>
#include <QTimer>

#include <QWidget>
#include <QFrame>
#include <QScrollArea>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QResizeEvent>

class PagedContent; // defined below
struct Page;        // defined in viewer_paged.cpp

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
    PagedContent(PagedContentViewer *parent);
    ~PagedContent();

    void clear();
    void recalculateArea();
    void reservePages(int numPages);
    void setPageImage(int num, const QImage &image);
    void setPageSize(int num, const QSize &size);

private:
    void moveEvent(QMoveEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    QWidget *viewport;
    QList<Page*> pages;
    QList<Page*> visiblePages;
    QTimer *moveTimer;
    bool isMoving;

private slots:
    void prepare();
    void stoppedMoving();

signals:
    void pageRequested(int num);
};

#endif /* VIEWER_PAGED_H */
