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
 * Find an external program to use with runHelper().
 *
 * This first checks the settings for a path specified by the user.
 * If none was set, it defaults to the specified fallback, which
 * becomes the new setting if it is found.
 *
 * If no valid executable is found, any existing setting is cleared.
 *
 * Returns the full path of the executable if the program is found,
 * or an empty string otherwise.
 */
QString Renderer::findHelper(const QString &settingName,
                             const QString &fallback)
{
    QSettings settings;
    QString program = settings.value(settingName, fallback).toString();
    if (QFileInfo(program).isExecutable()) {
        if (!settings.contains(settingName))
            settings.setValue(settingName, program);
    } else {
        settings.remove(settingName);
        program.clear();
    }
    return program;
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

/*
 * We enforce a lower limit on these values to keep our page geometry
 * from getting spicy, but otherwise we don't question them. If the
 * caller wants these large enough that something crashes or overflows,
 * that's their problem.
 */

void PagedContentRenderer::setPixelDensity(int dpiX, int dpiY)
{
    if (dpiX > 0 && dpiY > 0)
        dpiX_ = dpiX, dpiY_ = dpiY;
}

void PagedContentRenderer::setZoomFactor(int percent)
{
    if (percent > 0)
        zoomFactor_ = percent;
}
