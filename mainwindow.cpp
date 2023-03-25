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

#include <QtCore>
#include <QtWidgets>
#include <QMessageBox>

#include "renamifier.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f)
    : QMainWindow(parent, f)
{
    viewer = new Viewer(this);
    setCentralWidget(viewer);

    connect(viewer, &Viewer::renderProgress,
            this, &MainWindow::displayRenderProgress);

    createMenus();
    createToolBar();
    createActions();  // some actions are attached to nameEntry in the toolbar

    currentFileIndex = -1;
    lastBrowseDir = QDir::homePath();
    // processRenameAndMove() sets the initial value of lastMoveDir

    updateWindowTitle();
    nameEntry->setFocus();
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    closeAll();
}

void MainWindow::addPath(const QString &path, bool recurseIntoSubdirs)
{
    QFileInfo fi(path);
    if (fi.isDir())
        addDir(fi.absoluteFilePath(), recurseIntoSubdirs);
    else if (fi.isReadable())
        addFile(fi.absoluteFilePath());

    updateGoMenu();
    updateWindowTitle();
}

#ifdef Q_OS_WIN
#   define BROWSE_FOR_DIR_LABEL "Select Folder"
#else
#   define BROWSE_FOR_DIR_LABEL "Select Directory"
#endif

void MainWindow::browseForDir()
{
    QString path = QFileDialog::getExistingDirectory(
        this,
        BROWSE_FOR_DIR_LABEL,
        lastBrowseDir
    );

    if (!path.isEmpty()) {
        closeAll();
        addPath(path, false);
        displayFile();
    }
}

#undef BROWSE_FOR_DIR_LABEL

void MainWindow::browseForFiles()
{
    QStringList pathList = QFileDialog::getOpenFileNames(
        this,
        "Select Files to Rename",
        lastBrowseDir
    );

    if (!pathList.isEmpty()) {
        closeAll();
        for (int i = 0; i < pathList.size(); ++i)
            addFile(pathList[i]);
        displayFile();
    }
}

void MainWindow::closeAll()
{
    viewer->clear();
    nameEntry->clear();
    fileNames.clear();
    currentFileIndex = -1;
}

void MainWindow::closeCurrent()
{
    viewer->clear();
    fileNames.removeAt(currentFileIndex);

    if (fileNames.isEmpty()) {
        currentFileIndex = -1;
        nameEntry->clear();
        QTimer::singleShot(0, &QApplication::quit);
    } else
        displayFile(currentFileIndex % fileNames.size());
}

void MainWindow::displayFile(int index)
{
    if (!fileNames.isEmpty()) {
        int fileCount = fileNames.size();
        if (index < 0)
            currentFileIndex = fileCount - 1;
        else if (index >= fileCount)
            currentFileIndex = 0;
        else
            currentFileIndex = index;

        QString path = fileNames[currentFileIndex];
        viewer->display(path);

        QFileInfo fi(path);
        nameEntry->setText(fi.completeBaseName());

        // Open the "Select Files" dialog in this file's directory
        lastBrowseDir = fi.path();

        updateGoMenu();
        updateWindowTitle();
        nameEntry->setFocus();
        nameEntry->selectAll();
    }
    statusBar()->clearMessage();
}

void MainWindow::displayNext()
{
    displayFile(currentFileIndex + 1);
}

void MainWindow::displayPrevious()
{
    displayFile(currentFileIndex - 1);
}

void MainWindow::addDir(const QString &path, bool recurseIntoSubdirs)
{
    QDir::Filters filters =
        (recurseIntoSubdirs ? QDir::AllEntries : QDir::Files)
        | QDir::NoDotAndDotDot;
    QDir::SortFlags sortFlags = QDir::Name | QDir::DirsLast;

    QFileInfoList entryInfoList =
        QDir(path).entryInfoList(filters, sortFlags);

    for (int i = 0; i < entryInfoList.size(); ++i) {
        QFileInfo fi = entryInfoList[i];
        if (fi.isDir())
            addDir(fi.filePath(), recurseIntoSubdirs);
        else
            addFile(fi.filePath());
    }
}

void MainWindow::addFile(const QString &path)
{
    // Skip Unix-style hidden files whose names start with a "." character.
    // The current renaming logic doesn't handle those well, and since those
    // are usually configuration files and other things that shouldn't be
    // renamed anyway, I don't see any real need to fix it.
    if (!QFileInfo(path).baseName().isEmpty())
        fileNames.append(path);
}

void MainWindow::createActions()
{
    // Main window actions
    actionFocusNameEntry = new QAction(this);
    actionFocusNameEntry->setShortcut(QKeySequence("Ctrl+L"));
    connect(actionFocusNameEntry, &QAction::triggered,
            this, &MainWindow::triggerFocusNameEntry);
    addAction(actionFocusNameEntry);

    actionStopRender = new QAction(this);
    actionStopRender->setShortcut(QKeySequence("Escape"));
    connect(actionStopRender, &QAction::triggered,
            this, &MainWindow::triggerStopRender);
    addAction(actionStopRender);

    // Name entry actions
    actionDisplayNext = new QAction(nameEntry);
    actionDisplayNext->setShortcut(QKeySequence("PgDown"));
    connect(actionDisplayNext, &QAction::triggered,
            this, &MainWindow::triggerDisplayNext);
    nameEntry->addAction(actionDisplayNext);

    actionDisplayPrevious = new QAction(nameEntry);
    actionDisplayPrevious->setShortcut(QKeySequence("PgUp"));
    connect(actionDisplayPrevious, &QAction::triggered,
            this, &MainWindow::triggerDisplayPrevious);
    nameEntry->addAction(actionDisplayPrevious);

    actionRenameOnly = new QAction(nameEntry);
    actionRenameOnly->setShortcut(QKeySequence("Shift+Return"));
    connect(actionRenameOnly, &QAction::triggered,
            this, &MainWindow::triggerRenameOnly);
    nameEntry->addAction(actionRenameOnly);

    actionRenameAndDisplayNext = new QAction(nameEntry);
    actionRenameAndDisplayNext->setShortcut(QKeySequence("Return"));
    connect(actionRenameAndDisplayNext, &QAction::triggered,
            this, &MainWindow::triggerRenameAndDisplayNext);
    nameEntry->addAction(actionRenameAndDisplayNext);

    // Numeric keypad variant of the above
    actionKpRenameOnly = new QAction(nameEntry);
    actionKpRenameOnly->setShortcut(QKeySequence("Shift+Enter"));
    connect(actionKpRenameOnly, &QAction::triggered,
            this, &MainWindow::triggerRenameOnly);
    nameEntry->addAction(actionKpRenameOnly);

    actionKpRenameAndDisplayNext = new QAction(nameEntry);
    actionKpRenameAndDisplayNext->setShortcut(QKeySequence("Enter"));
    connect(actionKpRenameAndDisplayNext, &QAction::triggered,
            this, &MainWindow::triggerRenameAndDisplayNext);
    nameEntry->addAction(actionKpRenameAndDisplayNext);
}

// Use the appropriate terminology for the platform
#ifdef Q_OS_WIN
#   define BROWSE_FOR_DIR_LABEL "Open Fol&der..."
#else
#   define BROWSE_FOR_DIR_LABEL "Open &Directory..."
#endif

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open Files...",
                        this,
                        &MainWindow::triggerBrowseForFiles,
                        QKeySequence("Ctrl+O"));
    fileMenu->addAction(BROWSE_FOR_DIR_LABEL,
                        this,
                        &MainWindow::triggerBrowseForDir);
    fileMenu->addSeparator();
    fileMenu->addAction("Rename and &Move...",
                        this,
                        &MainWindow::triggerRenameAndMove,
                        QKeySequence("Ctrl+M"));
    fileMenu->addSeparator();
    fileMenu->addAction("&Close",
                        this,
                        &MainWindow::triggerCloseCurrent,
                        QKeySequence("Ctrl+W"));
    fileMenu->addAction("E&xit",
                        this,
                        &MainWindow::triggerQuit,
                        QKeySequence("Ctrl+Q"));

    // Note this is populated by updateGoMenu()
    goMenu = menuBar()->addMenu("&Go");
    connect(goMenu, &QMenu::triggered, this, &MainWindow::triggerDisplayFile);

    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About...",
                        this,
                        &MainWindow::triggerShowAbout);
}

#undef BROWSE_FOR_DIR_LABEL

void MainWindow::createToolBar()
{

    toolBar = new QToolBar(this);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    nameEntry = new QLineEdit(toolBar);

    // Try to use icons from the system theme when available
    QIcon iconPrevious = QIcon::fromTheme("go-previous",
                                          QIcon(":/icons/action_back.gif"));
    QIcon iconNext = QIcon::fromTheme("go-next",
                                      QIcon(":/icons/action_forward.gif"));
    QIcon iconRename = QIcon(":/icons/icon_wand.gif");

    toolBar->addAction(iconPrevious,
                       "Previous",
                       this,
                       &MainWindow::triggerDisplayPrevious);
    toolBar->addAction(iconNext,
                       "Next",
                       this,
                       &MainWindow::triggerDisplayNext);
    toolBar->addWidget(nameEntry);
    QAction *actionRename =
        toolBar->addAction(iconRename,
                           "Rename",
                           this,
                           &MainWindow::triggerRenameAndDisplayNext);
    addToolBar(Qt::TopToolBarArea, toolBar);

    // Display text on the Rename button since the icon alone may not make
    // its purpose immediately clear
    ((QToolButton*)toolBar->widgetForAction(actionRename))
        ->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void MainWindow::displayNextOrPromptToExit()
{
    if (currentFileIndex + 1 == fileNames.size()) {
        QMessageBox::StandardButton response = QMessageBox::question(
            this,
            "Done Renaming Files",
            "All files have been renamed. Exit Renamifier?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (response == QMessageBox::Yes)
            QTimer::singleShot(0, this, &QApplication::quit);
        else
            displayNext();
    } else
        displayNext();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        // Only accept the event if all paths specify local files
        bool allLocal = true;
        for (int i = 0; i < event->mimeData()->urls().size(); ++i) {
            if (!event->mimeData()->urls()[i].isLocalFile()) {
                allLocal = false;
                break;
            }
        }
        if (allLocal)
            event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        closeAll();
        for (int i = 0; i < event->mimeData()->urls().size(); ++i) {
            QUrl url = event->mimeData()->urls()[i];
            if (url.isLocalFile())
                addPath(url.toLocalFile());
        }
        displayFile();
    }
}

/*
 * Performs some basic sanity checks before a rename operation.
 * The operation should fail silently if this returns false.
 *
 * Basically, this is a failsafe to prevent things that shouldn't be possible
 * in the first place, like calling processRename() with no file displayed.
 * This would cause an invalid memory access attempt and crash the application.
 *
 * Note we explicitly do NOT validate user input here, since this is intended
 * only to make impossible operations fail silently. For example, invalid
 * filenames should be handled in rename_() rather than here so it can display
 * an appropriate error message and give the user a chance to correct them.
 *
 * Returns true if it's safe to proceed, false otherwise.
 */
bool MainWindow::readyToRename()
{
    return !(fileNames.isEmpty()
             || currentFileIndex < 0
             || currentFileIndex >= fileNames.size());
}

/*
 * Process a simple rename operation.
 *
 * Returns true if the rename succeeded, false otherwise.
 */
bool MainWindow::processRename()
{
    if (!readyToRename())
        return false;

    QString dstName = nameEntry->text();
    if (dstName.isEmpty())
        return false;  // the user probably doesn't need an error message
                       // to see the problem here

    QString srcPath = fileNames[currentFileIndex];
    QFileInfo fi(srcPath);
    QString suffix = fi.suffix();
    if (!suffix.isEmpty())
        dstName += "." + suffix;

    // In this case we always rename the file in its original directory
    QDir dstDir = fi.dir();
    QString dstPath = dstDir.filePath(dstName);

    return rename_(srcPath, dstPath);
}

/*
 * Process a rename-and-move operation.
 *
 * Returns true if the rename succeeded, false otherwise.
 */
bool MainWindow::processRenameAndMove()
{
    if (!readyToRename())
        return false;

    QString srcPath = fileNames[currentFileIndex];
    if (lastMoveDir.isEmpty())
        lastMoveDir = QFileInfo(srcPath).path();

    // Open the dialog in the last directory we moved a file to,
    // and suggest whatever name is currently in nameEntry
    QString dstName = nameEntry->text();
    QString suffix = QFileInfo(srcPath).suffix();
    if (!suffix.isEmpty())
        dstName += "." + suffix;
    QString dstNameSuggestion = QDir(lastMoveDir).filePath(dstName);

    QString dstPath = QFileDialog::getSaveFileName(
        this,
        "Rename and Move",
        dstNameSuggestion
    );

    if (dstPath.isEmpty())
        return false;
    else {
        // Always preserve the original file extension
        if (QFileInfo(dstPath).suffix() != suffix)
            dstPath += "." + suffix;
        if (rename_(srcPath, dstPath)) {
            // Open the next dialog in the directory where we moved this file
            lastMoveDir = QFileInfo(dstPath).path();
            return true;
        } else
            return false;
    }
}

/*
 * Rename the file, or display an error message if the operation failed.
 *
 * Returns true if the rename succeeded, false otherwise.
 */
bool MainWindow::rename_(const QString &srcPath, const QString &dstPath)
{
    // Don't do anything if the source and destination paths are the same
    if (srcPath == dstPath)
        return false;

    // Stop any active render, since that may have placed a lock on the file
    viewer->stopRender();

    QFile srcFile(srcPath);
    if (srcFile.rename(dstPath)) {
        // Update the list of open files
        fileNames[currentFileIndex] = dstPath;
        return true;
    } else {
        QString message;
        QTextStream(&message)
            << "Unable to rename \""
            << QFileInfo(srcPath).fileName() << "\".\n"
            << "\n"
            << srcFile.errorString();
        QMessageBox::critical(this, "Error", message);
        return false;
    }
}

/*
 * Re-populate the "Go" menu.
 *
 * Call this any time fileNames and/or currentFileIndex changes.
 */
void MainWindow::updateGoMenu()
{
    goMenu->clear();
    goMenu->addAction("&Previous File",
                      this,
                      &MainWindow::triggerDisplayPrevious,
                      QKeySequence("Back"));
    goMenu->addAction("&Next File",
                      this,
                      &MainWindow::triggerDisplayNext,
                      QKeySequence("Forward"));
    goMenu->addSeparator();

    // Note triggerDisplayFile() is connected to goMenu's triggered()
    // signal, which passes the QAction that triggered it, rather than
    // to actionDisplayFile's, which only passes a bool value indicating
    // whether the item was checked. N.B. we need the QAction there
    // because its data() contains the index of the corresponding file.
    QAction *actionDisplayFile;
    for (int i = 0; i < fileNames.size(); ++i) {
        QFileInfo fi(fileNames[i]);
        actionDisplayFile = goMenu->addAction(fi.fileName());
        actionDisplayFile->setCheckable(true);

        // Store the file index as this action's data
        actionDisplayFile->setData(i);

        // Check the item corresponding to the currently displayed file
        if (i == currentFileIndex)
            actionDisplayFile->setChecked(true);
    }
}

void MainWindow::updateWindowTitle()
{
    QString title;
    if (fileNames.isEmpty())
        QTextStream(&title) << "Renamifier";
    else
        QTextStream(&title) << "Renamifier - "
                            << currentFileIndex + 1
                            << " of " << fileNames.size();
    setWindowTitle(title);
}

void MainWindow::displayRenderProgress(int pagesDone, int pagesTotal)
{
    QString message;
    if (pagesDone == pagesTotal)
        statusBar()->clearMessage();
    else {
        if (pagesDone == 0)
            QTextStream(&message) << "Rendering started.";
        else
            QTextStream(&message) << "Rendered " << pagesDone
                                  << " of " << pagesTotal
                                  << " page" << ((pagesTotal == 1) ? "" : "s")
                                  << " (press Esc to interrupt).";
        statusBar()->showMessage(message);
    }
}

void MainWindow::triggerBrowseForDir(bool checked)
{
    (void)checked;
    browseForDir();
}

void MainWindow::triggerBrowseForFiles(bool checked)
{
    (void)checked;
    browseForFiles();
}

void MainWindow::triggerCloseCurrent(bool checked)
{
    (void)checked;
    closeCurrent();
}

void MainWindow::triggerDisplayFile(QAction *action)
{
    // We're only interested in this QAction if its data() contains an
    // int value, which is the index of the file to display. Actions
    // without associated data, like the "Previous" and "Next" commands,
    // are handled by their own trigger slots.
    if (action->data().canConvert(QMetaType::Int))
        displayFile(action->data().toInt());
}

void MainWindow::triggerDisplayNext(bool checked)
{
    (void)checked;
    displayNext();
}

void MainWindow::triggerDisplayPrevious(bool checked)
{
    (void)checked;
    displayPrevious();
}

void MainWindow::triggerFocusNameEntry(bool checked)
{
    (void)checked;
    nameEntry->setFocus();
    nameEntry->selectAll();
}

void MainWindow::triggerQuit(bool checked)
{
    (void)checked;
    QTimer::singleShot(0, this, &QApplication::quit);
}

void MainWindow::triggerRenameOnly(bool checked)
{
    (void)checked;
    if (processRename()) {
        updateGoMenu();
        // This provides a subtle indication that we've done something
        nameEntry->setFocus();
        nameEntry->selectAll();
    }
}

void MainWindow::triggerRenameAndDisplayNext(bool checked)
{
    (void)checked;
    if (processRename())
        displayNextOrPromptToExit();
}

void MainWindow::triggerRenameAndMove(bool checked)
{
    (void)checked;
    if (processRenameAndMove())
        displayNextOrPromptToExit();
}

void MainWindow::triggerShowAbout(bool checked)
{
    (void)checked;
    QString details;
    QTextStream(&details)
        << "<h1>Renamifier " VERSION "</h1>"
        << COPYRIGHT_HTML
        << LICENSE_TEXT_HTML;
    QMessageBox::about(this, "About Renamifier", details);
}

void MainWindow::triggerStopRender(bool checked)
{
    (void)checked;
    viewer->stopRender();
    statusBar()->showMessage("Rendering interrupted.");
}
