/**
 * @licence app begin@
 * Copyright (C) 2021 Alexander Wenzel
 *
 * This file is part of the DLT Relais project.
 *
 * \copyright This code is licensed under GPLv3.
 *
 * \author Alexander Wenzel <alex@eli2.de>
 *
 * \file settingsdialog.h
 * @licence end@
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "dltminiserver.h"
#include "dltcan.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void restoreSettings(DLTCan *dltCan, DLTMiniServer *dltMiniServer);
    void backupSettings(DLTCan *dltCan, DLTMiniServer *dltMiniServer);

private slots:

    void on_pushButtonAutoload_clicked();

    void on_checkBoxAutostart_clicked(bool checked);

    void on_groupBoxAutoload_clicked(bool checked);

private:

    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
