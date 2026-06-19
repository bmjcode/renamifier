/*
 * Renderer for PDF documents.
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
#include <QApplication>

#include "render_pdf.h"

static QString popplerError;

static const QString findGhostscript();
static const QString findGhostXPS();
static const QString findInSystemPath(const QString &fileName);
static void storePopplerError(const QString &message, const QVariant &closure);

PDFRenderer::PDFRenderer()
    : Renderer()
{
    document = nullptr;
    popplerError.clear();
}

void PDFRenderer::load()
{
    if (mimeType_.inherits("application/pdf"))
        document = Poppler::Document::load(path_);
    else if (mimeType_.inherits("application/postscript"))
        document = Poppler::Document::loadFromData(convertFromPostscript());
    else if (mimeType_.inherits("application/oxps")
             || mimeType_.inherits("application/xps"))
        document = Poppler::Document::loadFromData(convertFromXPS());
}

void PDFRenderer::init()
{
    Poppler::setDebugErrorFunction(&storePopplerError, QVariant());
}

QSize PDFRenderer::pageSize(int num) const
{
    if (document != nullptr) {
        std::unique_ptr<Poppler::Page> page = document->page(num);
        if (page != nullptr) {
            QSize pointSize = page->pageSize();
            // Convert points to pixels at our current DPI
            return zoomScaled(QSize(pointSize.width() * dpiX_ / 72,
                                    pointSize.height() * dpiY_ / 72));
        }
    }
    return QSize(0, 0);
}

void PDFRenderer::renderPage(int num)
{
    int xRes = zoomScaled(dpiX_), yRes = zoomScaled(dpiY_);

    // Check whether an error occurred while rendering this document
    if (document == nullptr) {
        if (popplerError.isEmpty()) {
            // Renderer::create() should prevent this from ever happening,
            // but we should handle it just in case...
            QString message;
            QTextStream(&message)
                << "Invalid MIME type for PDFRenderer: "
                << mimeType_.name();
            renderError(message);
        } else
            renderError(popplerError);
        return;
    }

    // Make the document look nice on screen
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    emit renderMode(PagedContent);
    emit renderedPage(num, document->page(num)->renderToImage(xRes, yRes));
}

QByteArray PDFRenderer::convertFromPostscript()
{
    QString program = findGhostscript();
    if (program.isEmpty()) {
        renderError("Cannot display this file because Ghostscript "
                    "is not installed.");
        return nullptr;
    }

    QStringList arguments;
    arguments << "-q"
              << "-dBATCH"
              << "-dNOPAUSE"
              << "-dSAFER"
              << "-sDEVICE=pdfwrite"
              << "-sOutputFile=-"
              << path_;

    return runHelper(program, arguments);
}

QByteArray PDFRenderer::convertFromXPS()
{
    QString program = findGhostXPS();
    if (program.isEmpty()) {
        renderError("Cannot display this file because GhostXPS "
                    "is not installed.");
        return nullptr;
    }

    QStringList arguments;
    arguments << "-dNOPAUSE"
              << "-sDEVICE=pdfwrite"
              << "-sOutputFile=-"
              << path_;

    return runHelper(program, arguments);
}

/*
 * Stores debug and error messages from Poppler so we can display them
 * in the application.
 */
void storePopplerError(const QString &message, const QVariant &closure)
{
    (void)closure;
    if (!popplerError.isEmpty())
        popplerError += "\n";
    popplerError += message;
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
                    break;
                }
            }
            if (!program.isEmpty())
                break;
        }
#else /* Q_OS_WIN */
        program = findInSystemPath("gxps");
#endif /* Q_OS_WIN */
    }
    return program;
}

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
