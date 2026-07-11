/*
 * Functions for rendering using external "helper" programs.
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

#ifndef RENDERER_HELPERS_H
#define RENDERER_HELPERS_H

#include <QByteArray>
#include <QPair>
#include <QString>
#include <QStringList>

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
const QString findHelper(const QString &settingName,
                         const QString &fallback = QString());

/*
 * Locate a program in the system's $PATH.
 *
 * Returns the full path to the program executable if found, or an empty
 * string otherwise.
 */
const QString findInSystemPath(const QString &fileName);

/*
 * Run an external program to convert a file into something we can display.
 *
 * Returns a QPair<bool, QByteArray> where:
 *   .first indicates whether the program ran successfully, and
 *   .second contains its output (including standard error in case of failure).
 */
using HelperResult = QPair<bool, QByteArray>;
HelperResult runHelper(const QString &program, const QStringList &arguments);

#endif /* RENDERER_HELPERS_H */
