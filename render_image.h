/*
 * A simple image renderer.
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

#ifndef RENDER_IMAGE_H
#define RENDER_IMAGE_H

#include <QObject>
#include <QSize>
#include <QImage>

#include "renderer.h"

class ImageRenderer : public Renderer {
    Q_OBJECT

public:
    ImageRenderer();

    inline int numPages() const { return 1; }
    inline QSize pageSize(int num) const
        { return zoomScaled(image.size()); }

    void load();
    void renderPage(int num);

private:
    QImage image;
};

#endif /* RENDER_IMAGE_H */
