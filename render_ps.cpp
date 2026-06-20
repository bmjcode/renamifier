/*
 * Renderer for Postscript documents.
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

#include "render_ps.h"
#include "renderer_util.h"

static const QString findGhostscript();

PSRenderer::PSRenderer()
    : PDFRenderer()
{
}

void PSRenderer::load()
{
    QString program = findGhostscript();
    if (program.isEmpty()) {
        renderError("Cannot display this file because Ghostscript "
                    "is not installed.");
        return;
    }

    QStringList arguments;
    arguments << "-q"
              << "-dBATCH"
              << "-dNOPAUSE"
              << "-dSAFER"
              << "-sDEVICE=pdfwrite"
              << "-sOutputFile=-"
              << path_;

    loadFromData(runHelper(program, arguments));
}

/*
 * Helper function to return the path to the Ghostscript executable.
 */
const QString findGhostscript()
{
    static QString program;
    if (program.isEmpty()) {
#ifdef Q_OS_WIN
        // Possible names for the Ghostscript executable
        // Note we include all possible names regardless of the target
        // platform so that a 32-bit Renamifier can still find a 64-bit
        // Ghostscript if running on such a system.
        QStringList gsNames;
        gsNames << "gswin64c.exe"
                << "gswin32c.exe";

        // Possible names for %ProgramFiles%
        QStringList pfDirs;
        pfDirs << std::getenv("ProgramFiles")
               << std::getenv("ProgramW6432")
               << std::getenv("ProgramFiles(x86)");

        // Ghostscript is usually found under a versioned path like
        // %ProgramFiles%\gs\gs9.27\bin
        for (int i = 0; i < pfDirs.size(); ++i) {
            if (pfDirs[i].isEmpty())
                continue;
            // Search for Ghostscript installations
            QDir gsBaseDir(pfDirs[i] + "/gs");
            QStringList gsDirs =
                gsBaseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < gsDirs.size(); ++j) {
                // See what executables this installation provides
                for (int k = 0; k < gsNames.size(); ++k) {
                    QString gsExe = gsBaseDir.path() + "/"
                                    + gsDirs[j] + "/bin/" + gsNames[k];
                    if (QFileInfo(gsExe).isExecutable()) {
                        program = gsExe;
                        break;
                    }
                }
                if (!program.isEmpty())
                    break;
            }
        }
#else /* Q_OS_WIN */
        program = findInSystemPath("gs");
#endif /* Q_OS_WIN */
    }
    return program;
}
