/*
 * Renderer for XPS documents.
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
#include <QApplication>

#include "render_xps.h"
#include "renderer_util.h"

static const QString findGhostXPS();

XPSRenderer::XPSRenderer()
    : PDFRenderer()
{
}

bool XPSRenderer::load()
{
    QString program = findHelper("helpers/gxps", findGhostXPS());
    if (program.isEmpty()) {
        storeLoadError("Cannot display this file because GhostXPS "
                       "is not installed.");
        return false;
    }

    QStringList arguments;
    arguments << "-dNOPAUSE"
              << "-sDEVICE=pdfwrite"
              << "-sOutputFile=-"
              << path();

    return loadFromData(runHelper(program, arguments));
}

/*
 * Helper function to return the path to the GhostXPS executable.
 */
const QString findGhostXPS()
{
    static QString program;
    if (program.isEmpty()) {
#ifdef Q_OS_WIN
        // Possible names for the GhostXPS executable
        QStringList gxpsNames;
        gxpsNames << "gxpswin64.exe"
                  << "gxpswin32.exe";

        // GhostXPS does not currently provide its own installer,
        // so on Windows we'll ship our own copy with the application
        QDir gxpsBaseDir(QApplication::applicationDirPath());
        QStringList gxpsDirs =
            gxpsBaseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (int i = 0; i < gxpsDirs.size(); ++i) {
            for (int j = 0; j < gxpsNames.size(); ++j) {
                QString gxpsExe = gxpsBaseDir.path() + "/"
                                  + gxpsDirs[i] + "/" + gxpsNames[j];
                if (QFileInfo(gxpsExe).isExecutable()) {
                    program = gxpsExe;
                    goto finished;
                }
            }
        }
#else /* Q_OS_WIN */
        program = findInSystemPath("gxps");
#endif /* Q_OS_WIN */
    }
finished:
    return program;
}
