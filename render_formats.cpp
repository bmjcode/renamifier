/*
 * Support for reading various file formats.
 * Copyright (c) 2021, 2025 Benjamin Johnson
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

#include <cctype>
#include <cstdlib>
#include <memory>

#include <QtCore>
#include <QApplication>
#include <QImage>
#include <poppler-qt6.h>

#include "render_formats.h"

static QString popplerError;

static const QString findGhostscript();
static const QString findGhostXPS();
static const QString findInSystemPath(const QString &fileName);
static const QString hexDump(QIODevice &device);
static void storePopplerError(const QString &message, const QVariant &closure);

ImageRenderer::ImageRenderer()
    : Renderer()
{
}

void ImageRenderer::render()
{
    QImage image(path_);
    if (image.isNull()) {
        renderError();
        return;
    }

    emit renderMode(PagedContent);
    if (mimeType_.inherits("image/tiff")) {
        QString warning;
        QTextStream(&warning)
            << "Displaying page 1 of file:"
            << Qt::endl
            << path_
            << Qt::endl
            << Qt::endl
            << "Please note that support for multi-page TIFF documents "
               "is not yet implemented.";
        emit renderedText(warning);
    }
    emit renderedImage(image);
}

PDFRenderer::PDFRenderer()
    : Renderer()
{
    popplerError.clear();
}

void PDFRenderer::init()
{
    Poppler::setDebugErrorFunction(&storePopplerError, QVariant());
}

void PDFRenderer::render()
{
    std::unique_ptr<Poppler::Document> document;

    if (mimeType_.inherits("application/pdf"))
        document = Poppler::Document::load(path_);
    else if (mimeType_.inherits("application/postscript"))
        document = Poppler::Document::loadFromData(convertFromPostscript());
    else if (mimeType_.inherits("application/oxps")
        || mimeType_.inherits("application/xps"))
        document = Poppler::Document::loadFromData(convertFromXPS());

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

    numPages_ = document->numPages();
    emit renderMode(PagedContent);
    emit renderProgress(0, numPages_);

    for (int i = 0; i < numPages_; ++i) {
        if (QThread::currentThread()->isInterruptionRequested())
            break;

        std::unique_ptr<Poppler::Page> page = document->page(i);
        QImage image = page->renderToImage(dpiX_, dpiY_);

        if (image.isNull()) {
            QString message;
            QTextStream(&message) << "Failed to render page " << i + 1 << ".";
            emit renderedText(message);
        } else
            emit renderedPage(image);
        emit renderProgress(i + 1, numPages_);
    }
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

TextRenderer::TextRenderer()
    : Renderer()
{
}

void TextRenderer::render()
{
    QFile file(path_);
    QString contents;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream(&contents) << file.readAll();
        file.close();
        emit renderMode(TextContent);
        emit renderedText(contents);
    } else
        renderError(file.errorString());
}

UnknownFormatRenderer::UnknownFormatRenderer()
    : Renderer()
{
}

void UnknownFormatRenderer::render()
{
    QFile file(path_);
    QString output = hexDump(file);
    if (output.isEmpty()) {
        renderError(file.errorString());
        return;
    } else {
        QString message;
        QTextStream(&message)
            << "Unable to find a suitable renderer for this file:"
            << Qt::endl
            << path_
            << Qt::endl
            << Qt::endl
            << "Its detected MIME type is " << mimeType_.name() << "."
            << Qt::endl
            << "Please include this information if you submit a "
               "feature request or bug report."
            << Qt::endl
            << Qt::endl
            << output;
        emit renderMode(TextContent);
        emit renderedText(message);
    }
}

/*
 * Returns a hex dump of the specified file in the style of xxd(1).
 */
const QString hexDump(QIODevice &device)
{
    QString output;
    QTextStream outputStream(&output);
    outputStream.setPadChar('0');

    outputStream << Qt::hex;
    if (device.open(QIODevice::ReadOnly)) {
        char c[16];
        int bytesRead, position;

        position = 0;
        while (!device.atEnd()) {
            // Output the stream position
            outputStream.setFieldWidth(8);
            outputStream << position;
            outputStream.setFieldWidth(0);
            outputStream << ": ";

            // Read a line of up to 16 bytes
            bytesRead = device.read(c, 16);
            position += bytesRead;

            // Output the hex dump in two-byte columns
            for (int i = 0; i < 16; ++i) {
                outputStream.setFieldWidth(2);
                if (i < bytesRead)
                    outputStream << (uint8_t)c[i];
                else
                    outputStream << "  ";  // pad lines shorter than 16 chars

                // Add one space between columns, and two spaces between the
                // final column and the human-readable version
                outputStream.setFieldWidth(0);
                if ((i + 1) % 2 == 0)
                    outputStream << " ";
                if ((i + 1) % 16 == 0)
                    outputStream << " ";
            }

            // Output a human-readable version containing all the printable
            // characters in this line
            for (int i = 0; i < std::min(bytesRead, 16); ++i)
                outputStream << (std::isprint(c[i]) ? c[i] : '.');
            outputStream << Qt::endl;
        }
        device.close();
    } else
        output = device.errorString();
    return output;
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
