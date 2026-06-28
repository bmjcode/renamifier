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

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QObject>
#include <QString>

#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class PathEdit;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent);

public slots:
    void accept();

private:
    QVBoxLayout *mainLayout;

    QGroupBox *helperGroupBox;
    QGridLayout *helperLayout;
    QLabel *gsLabel;
    PathEdit *gsPathEdit;
    QLabel *gxpsLabel;
    PathEdit *gxpsPathEdit;

    QHBoxLayout *buttonLayout;
    QPushButton *buttonOK;
    QPushButton *buttonCancel;

    void createHelperSettings();
    void createButtons();
    void loadSettings();
    void saveSettings();
};

class PathEdit : public QWidget {
    Q_OBJECT

public:
    PathEdit(QWidget *parent);

    QString path() const { return lineEdit->text(); }

public slots:
    inline void setDir(const QString &dir) { dir_ = dir; }
    inline void setFilter(const QString &filter) { filter_ = filter; }
    inline void setPath(const QString &path) { lineEdit->setText(path); }

private:
    QHBoxLayout *layout;
    QLineEdit *lineEdit;
    QPushButton *browseButton;
    QString dir_;
    QString filter_;

private slots:
    void browse();

signals:
    void returnPressed();
};

#endif /* SETTINGS_DIALOG_H */
