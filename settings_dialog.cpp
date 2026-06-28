/*
 * Renamifier's settings dialog.
 * Copyright (c) 2026 Benjamin Johnson
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

#include <cstdlib>  // for std::getenv()

#include <QtCore>
#include <QtWidgets>

#include "settings_dialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Options");

    mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    createHelperSettings();

    createButtons();
    loadSettings();
}

void SettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void SettingsDialog::createHelperSettings()
{
    QString initialBrowseDir;
    QString gsFilter;
    QString gxpsFilter;

#ifdef Q_OS_WIN
    initialBrowseDir = std::getenv("ProgramFiles");
    gsFilter = "Ghostscript executables (gswin??c.exe)";
    gxpsFilter = "GhostXPS executables (gxpswin??.exe)";
#else
    initialBrowseDir = "/usr/bin";
    gsFilter = "Ghostscript executables (gs)";
    gxpsFilter = "GhostXPS executables (gxps)";
#endif

    helperGroupBox = new QGroupBox("Helper Programs", this);
    mainLayout->addWidget(helperGroupBox);

    helperLayout = new QGridLayout(helperGroupBox);
    helperLayout->setColumnStretch(1, 1);
    helperGroupBox->setLayout(helperLayout);

    gsLabel = new QLabel("Ghostscript:", helperGroupBox);
    helperLayout->addWidget(gsLabel, 0, 0);

    gsPathEdit = new PathEdit(helperGroupBox);
    gsPathEdit->setDir(initialBrowseDir);
    gsPathEdit->setFilter(gsFilter);
    gsLabel->setBuddy(gsPathEdit);
    helperLayout->addWidget(gsPathEdit, 0, 1);

    gxpsLabel = new QLabel("GhostXPS:", helperGroupBox);
    helperLayout->addWidget(gxpsLabel, 1, 0);

    gxpsPathEdit = new PathEdit(helperGroupBox);
    gxpsPathEdit->setDir(initialBrowseDir);
    gxpsPathEdit->setFilter(gxpsFilter);
    gxpsLabel->setBuddy(gxpsPathEdit);
    helperLayout->addWidget(gxpsPathEdit, 1, 1);

    connect(gsPathEdit, &PathEdit::returnPressed,
            this, &SettingsDialog::accept);
    connect(gxpsPathEdit, &PathEdit::returnPressed,
            this, &SettingsDialog::accept);
}

void SettingsDialog::createButtons()
{
    buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(); // right-align buttons
    mainLayout->addLayout(buttonLayout);

    buttonOK = new QPushButton("OK", this);
    buttonOK->setDefault(true);
    buttonLayout->addWidget(buttonOK);

    buttonCancel = new QPushButton("Cancel", this);
    buttonLayout->addWidget(buttonCancel);

    connect(buttonOK, &QPushButton::clicked,
            this, &SettingsDialog::accept);
    connect(buttonCancel, &QPushButton::clicked,
            this, &SettingsDialog::reject);
}

void SettingsDialog::loadSettings()
{
    QSettings settings;

    gsPathEdit->setPath(settings.value("helpers/gs").toString());
    gxpsPathEdit->setPath(settings.value("helpers/gxps").toString());
}

void SettingsDialog::saveSettings()
{
    QSettings settings;

    QString gsPath = gsPathEdit->path();
    if (gsPath.isEmpty())
        settings.remove("helpers/gs");
    else
        settings.setValue("helpers/gs", gsPath);

    QString gxpsPath = gxpsPathEdit->path();
    if (gxpsPath.isEmpty())
        settings.remove("helpers/gxps");
    else
        settings.setValue("helpers/gxps", gxpsPath);
}

PathEdit::PathEdit(QWidget *parent)
    : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    setLayout(layout);

    lineEdit = new QLineEdit(this);
    layout->addWidget(lineEdit, 1);

    connect(lineEdit, &QLineEdit::returnPressed,
            this, &PathEdit::returnPressed);

    browseButton = new QPushButton("Browse...", this);
    layout->addWidget(browseButton);

    connect(browseButton, &QPushButton::clicked,
            this, &PathEdit::browse);
}

void PathEdit::browse()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Browse",
        dir_,
        filter_);
    if (!fileName.isEmpty())
        lineEdit->setText(fileName);
    lineEdit->setFocus();
}
