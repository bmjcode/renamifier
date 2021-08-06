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

#ifndef RENAMIFIER_TEST_H
#define RENAMIFIER_TEST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

#include <QtTest>

#include "mainwindow.h"

class RenamifierTest : public QObject
{
    Q_OBJECT

private:
    MainWindow *mainWindow;
    QStringList testFiles;
    QStringList tempFiles;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Tests for essential functionality
    void renameWorks();

    // Tests for correct UI behavior
    void displayFileWraps();
    void renameWithNoNameEntered();

    // Tests for uncommon or perverse situations
    void displayFileWithNothingOpen();
    void renameWithNothingOpen();
    void renameAndMoveWithNothingOpen();

private:
    void addTestFiles();
    void confirmThatNothingIsOpen();
    void confirmThatFileIsDisplayed(int index);
    QTemporaryFile* renameTestFile();
};

#endif /* RENAMIFIFER_TEST_H */
