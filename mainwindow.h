/*
 * Renamifier's main window.
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <QAction>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QProgressBar>
#include <QToolBar>

#include "viewer.h"

/*
 * The application's main window.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr,
               Qt::WindowFlags f = Qt::WindowFlags());
    ~MainWindow();

    void addPath(const QString &path, bool recurseIntoSubdirs = false);
    void browseForDir();
    void browseForFiles(bool quitIfCanceled = false);
    void closeAll();
    void closeCurrent();
    void displayFile(int index = 0);
    void displayNext();
    void displayPrevious();

protected:
    Viewer *viewer;
    QMenu *fileMenu;
    QMenu *goMenu;
    QMenu *helpMenu;
    QToolBar *toolBar;
    QLineEdit *nameEntry;
    QAction *actionDisplayNext;
    QAction *actionDisplayPrevious;
    QAction *actionFocusNameEntry;
    QAction *actionRenameOnly;
    QAction *actionRenameAndDisplayNext;
    QAction *actionKpRenameOnly;
    QAction *actionKpRenameAndDisplayNext;
    QAction *actionStopRender;

    int currentFileIndex;
    QStringList fileNames;
    QString lastBrowseDir;
    QString lastMoveDir;

    void addDir(const QString &path, bool recurseIntoSubdirs);
    void addFile(const QString &path);
    void createActions();
    void createMenus();
    void createToolBar();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    // The rename methods are protected because they should only be triggered
    // interactively by the user, not programatically. There is an exception
    // for test.cpp, which uses a subclass to expose the internals it needs for
    // unit testing (this is also why we use protected rather than private).
    bool processRename();
    bool processRenameAndMove();
    bool readyToRename();
    bool rename_(const QString &srcPath, const QString &dstPath);
    void updateGoMenu();
    void updateWindowTitle();

protected slots:
    void displayRenderProgress(int pagesDone, int pagesTotal);
    void triggerBrowseForDir(bool checked);
    void triggerBrowseForFiles(bool checked);
    void triggerCloseCurrent(bool checked);
    void triggerDisplayFile(QAction *action);
    void triggerDisplayNext(bool checked);
    void triggerDisplayPrevious(bool checked);
    void triggerFocusNameEntry(bool checked);
    void triggerQuit(bool checked);
    void triggerRenameOnly(bool checked);
    void triggerRenameAndDisplayNext(bool checked);
    void triggerRenameAndMove(bool checked);
    void triggerShowAbout(bool checked);
    void triggerStopRender(bool checked);

private:
    friend class RenamifierTest;
};

#endif /* MAINWINDOW_H */
