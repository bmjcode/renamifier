/*
 * Viewer utility classes and functions.
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

#ifndef VIEWER_UTIL_H
#define VIEWER_UTIL_H

#include <QObject>
#include <QPoint>

#include <QScrollArea>
#include <QResizeEvent>
#include <QWheelEvent>

/*
 * A customized QScrollArea with some quality-of-life enhancements.
 */
class ViewerScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    ViewerScrollArea(QWidget *parent);

    QPoint scrollBarPosition() const;
    void setScrollBarPosition(int x, int y);
    inline void setScrollBarPosition(const QPoint &point)
        { setScrollBarPosition(point.x(), point.y()); }

private:
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

signals:
    void wheelZoomed(int delta);
};

#endif /* VIEWER_UTIL_H */
