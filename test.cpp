/*
 * Test cases for Renamifier.
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

#include "test.h"

/*
 * Initialize the test case.
 */
void
RenamifierTest::initTestCase()
{
    // Keep this in sync with test.qrc
    testFiles
        << "CMakeLists.txt"
        << "COPYING"
        << "mainwindow.cpp"
        << "mainwindow.h"
        << "renamifier.cpp"
        << "renamifier.h"
        << "renamifier.qrc"
        << "renderer.cpp"
        << "renderer.h"
        << "test.cpp"
        << "test.h"
        << "test.qrc"
        << "viewer.cpp"
        << "viewer.h";
}

/*
 * Clean up after the test case.
 */
void
RenamifierTest::cleanupTestCase()
{
}

/*
 * Initialize each individual test.
 */
void
RenamifierTest::init()
{
    mainWindow = new MainWindow;
}

/*
 * Clean up after each individual test.
 */
void
RenamifierTest::cleanup()
{
    // Clean up temporary files
    for (int i = 0; i < tempFiles.size(); ++i)
        QFile(tempFiles[i]).remove();
    tempFiles.clear();

    delete mainWindow;
}

/*
 * Test that the rename operation works.
 */
void
RenamifierTest::renameWorks()
{
    QTemporaryFile *srcFile = renameTestFile();
    QString srcPath = srcFile->fileName();
    delete srcFile;  // to remove any lock QTemporaryFile may have on this file

    QFileInfo srcFileInfo(srcPath);
    QString srcBaseName = srcFileInfo.completeBaseName();
    QString dstBaseName = srcBaseName;
    QString suffix = srcFileInfo.suffix();

    // Generate a suitable base name for the destination file
    dstBaseName.replace("test.", "renamed.");
    QString dstPath = srcFileInfo.dir().filePath(
        suffix.isEmpty() ? dstBaseName : dstBaseName + "." + suffix
    );

    // Add both old and new names to the cleanup list in case the test fails
    tempFiles.append(srcPath);
    tempFiles.append(dstPath);

    // Confirm that the source exists and the destination doesn't
    QVERIFY(QFileInfo(srcPath).exists());
    QVERIFY(!QFileInfo(dstPath).exists());

    // Add and display our test file
    mainWindow->addPath(srcPath);
    mainWindow->displayFile();
    confirmThatFileIsDisplayed(0);

    // Simulate entering a new base name and clicking "Rename"
    mainWindow->nameEntry->setText(dstBaseName);
    QVERIFY(mainWindow->processRename());

    // Confirm that the destination exists and the source doesn't
    QVERIFY(!QFileInfo(srcPath).exists());
    QVERIFY(QFileInfo(dstPath).exists());
}

/*
 * Test that MainWindow::displayFile() wraps around.
 */
void
RenamifierTest::displayFileWraps()
{
    addTestFiles();
    int numFileNames = mainWindow->fileNames.size();

    // Simulates clicking "Previous" when the first file is displayed
    mainWindow->displayFile(-1);
    QCOMPARE(mainWindow->currentFileIndex, numFileNames - 1);

    // Simulates clicking "Next" when the last file is displayed
    mainWindow->displayFile(numFileNames);
    QCOMPARE(mainWindow->currentFileIndex, 0);
}

/*
 * Test that the rename operation fails silently when no new name is entered.
 */
void
RenamifierTest::renameWithNoNameEntered()
{
    QTemporaryFile *srcFile = renameTestFile();
    QString srcPath = srcFile->fileName();
    delete srcFile;

    tempFiles.append(srcPath);

    // Confirm that the source exists
    QVERIFY(QFileInfo(srcPath).exists());

    // Add and display our test file
    mainWindow->addPath(srcPath);
    mainWindow->displayFile();
    confirmThatFileIsDisplayed(0);

    // Simulate blanking the name entry and clicking "Rename"
    mainWindow->nameEntry->clear();
    QVERIFY(!mainWindow->processRename());

    // Confirm that the source still exists
    QVERIFY(QFileInfo(srcPath).exists());
}

/*
 * Test that MainWindow::displayFile() does nothing when no files are open.
 */
void
RenamifierTest::displayFileWithNothingOpen()
{
    confirmThatNothingIsOpen();
    QCOMPARE(mainWindow->currentFileIndex, -1);

    mainWindow->displayFile();

    // Make sure nothing changed
    confirmThatNothingIsOpen();
    QCOMPARE(mainWindow->currentFileIndex, -1);
}

/*
 * Test that MainWindow::rename() does nothing when no files are open.
 */
void
RenamifierTest::renameWithNothingOpen()
{
    confirmThatNothingIsOpen();
    QVERIFY(!mainWindow->readyToRename());

    // The real test is that this doesn't crash the program
    QVERIFY(!mainWindow->processRename());
}

/*
 * Test that MainWindow::renameAndMove() does nothing when no files are open.
 */
void
RenamifierTest::renameAndMoveWithNothingOpen()
{
    confirmThatNothingIsOpen();
    QVERIFY(!mainWindow->readyToRename());

    // The real test is that this doesn't crash the program
    QVERIFY(!mainWindow->processRenameAndMove());
}

/*
 * Add some test files.
 */
void
RenamifierTest::addTestFiles()
{
    for (int i = 0; i < testFiles.size(); ++i)
        mainWindow->addPath(":/" + testFiles[i]);
    QCOMPARE(mainWindow->fileNames.size(), testFiles.size());
}

/*
 * Confirm that no files are open.
 */
void
RenamifierTest::confirmThatNothingIsOpen()
{
    QVERIFY(mainWindow->fileNames.isEmpty());
    QCOMPARE(mainWindow->currentFileIndex, -1);
}

/*
 * Confirm that the file at fileNames[index] is displayed.
 */
void
RenamifierTest::confirmThatFileIsDisplayed(int index)
{
    QVERIFY(!mainWindow->fileNames.isEmpty());
    QCOMPARE(mainWindow->currentFileIndex, index);
}

/*
 * Returns a temporary file for testing rename operations.
 */
QTemporaryFile*
RenamifierTest::renameTestFile()
{
    QTemporaryFile *tempFile = new QTemporaryFile("test.XXXXXX.cpp");
    tempFile->setAutoRemove(false);

    // Use (our embedded copy of) this file's source code as the test data
    QFile input(":/test.cpp");

    // Write some sample data to this file
    if (tempFile->open()) {
        if (input.open(QIODevice::ReadOnly)) {
            tempFile->write(input.readAll());
            input.close();
        }
        tempFile->close();
    }
    return tempFile;
}

QTEST_MAIN(RenamifierTest)
