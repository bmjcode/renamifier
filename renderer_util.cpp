/*
 * Renderer utility functions.
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

#include <cstdlib>

#include <QtCore>

#include "renderer_util.h"

/*
 * Helper function to locate a program in the system's $PATH.
 *
 * Returns the full path to the program executable if found, or an empty
 * string otherwise.
 */
const QString findInSystemPath(const QString &fileName)
{
    QString program, candidate;
    QStringList paths = QString(std::getenv("PATH")).split(":");

    for (int i = 0; i < paths.size(); ++i) {
        candidate = QDir(paths[i]).filePath(fileName);
        if (QFileInfo(candidate).isExecutable()) {
            program = candidate;
            break;
        }
    }
    return program;
}
