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

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    Renderer::init();

    MainWindow window;
    window.show();

    QCommandLineParser parser;
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
        window.browseForFiles(true);
    else {
        QStringListIterator pathIterator(positionalArguments);
        while (pathIterator.hasNext())
            window.addPath(pathIterator.next());
        window.displayFile();
    }
    return app.exec();
}
