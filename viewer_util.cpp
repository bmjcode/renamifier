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

#include <algorithm>    // for std::max()

#include <QtCore>
#include <QtWidgets>

#include "viewer_util.h"

ViewerScrollArea::ViewerScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    setBackgroundRole(QPalette::Dark);
}

QPoint ViewerScrollArea::scrollBarPosition() const {
    return QPoint(
        horizontalScrollBar()->sliderPosition(),
        verticalScrollBar()->sliderPosition());
}

void ViewerScrollArea::setScrollBarPosition(int x, int y)
{
    horizontalScrollBar()->setSliderPosition(x);
    verticalScrollBar()->setSliderPosition(y);
}

/*
 * Resize the inner frame when the widget's size changes.
 */
void ViewerScrollArea::resizeEvent(QResizeEvent *event)
{
    widget()->resize(
        std::max(viewport()->width(), widget()->minimumWidth()),
        std::max(viewport()->height(), widget()->minimumHeight()));
}

void ViewerScrollArea::wheelEvent(QWheelEvent *event)
{
    // Adapted from QPlainTextEdit::wheelEvent()
    if (event->modifiers() & Qt::ControlModifier) {
        float delta = event->angleDelta().y() / 120.f;
        emit wheelZoomed(delta);
        return;
    }
    QScrollArea::wheelEvent(event);
}
