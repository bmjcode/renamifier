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

#include <QtCore>

#include "renderer.h"

/*
 * Construct a new Renderer.
 */
Renderer::Renderer()
{
    loaded_ = false;
}

/*
 * Run an external program to convert a file into something we can display.
 *
 * Returns the raw data as a QByteArray.
 */
QByteArray Renderer::runHelper(const QString &program,
                               const QStringList &arguments)
{
    QProcess helper;
    helper.start(program, arguments);
    if (helper.waitForFinished() && helper.exitCode() == 0)
        return helper.readAllStandardOutput();
    else {
        QString message;
        QTextStream(&message) << helper.readAll();
        if (loaded_)
            emit errorEncountered(message);
        else
            storeLoadError(message);
    }
    return nullptr;
}

TextContentRenderer::TextContentRenderer()
    : Renderer()
{
}

PagedContentRenderer::PagedContentRenderer()
    : Renderer()
{
    // Default to the DPI of a standard PC screen
    dpiX_ = dpiY_ = 96;
    zoomFactor_ = 100;
}
