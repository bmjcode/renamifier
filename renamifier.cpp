/*
 * Renamifier: A tool to preview and rename digital files.
 * Copyright (c) 2021 Benjamin Johnson
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

#include <QCoreApplication>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_WIN
#   include <QStyleFactory>
#endif

#include "main_window.h"
#include "renderer.h"

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName("Benjamin Johnson");
    QCoreApplication::setApplicationName("Renamifier");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

#ifdef Q_OS_WIN
    // This looks more native than the default style on Windows 11
    QApplication::setStyle(QStyleFactory::create("windowsvista"));
#endif

    Renderer::init();

    MainWindow window;
    window.show();

    QCommandLineParser parser;
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
        window.browseForFiles();
    else {
        QStringListIterator pathIterator(positionalArguments);
        while (pathIterator.hasNext()) {
            QFileInfo fileInfo(pathIterator.next());
            QDir dir = fileInfo.dir();
            QStringList nameFilters;

            // treat each positional argument as a glob() pattern
            nameFilters << fileInfo.fileName();
            QStringList matches = dir.entryList(nameFilters);

            QStringListIterator matchIterator(matches);
            while (matchIterator.hasNext())
                window.addPath(dir.filePath(matchIterator.next()));
        }
        window.displayFile();
    }
    return app.exec();
}
