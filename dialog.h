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
 * \file dialog.h
 * @licence end@
 */

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSettings>

#include "dltcan.h"
#include "dltminiserver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(bool autostart,QString configuration,QWidget *parent = nullptr);
    ~Dialog();

private slots:

    // Status of Relais and DLT connection
    void statusCan(QString text);
    void statusDlt(QString text);

    void injection(QString text);

    // Settings and Info
    void on_pushButtonSettings_clicked();
    void on_pushButtonDefaultSettings_clicked();
    void on_pushButtonLoadSettings_clicked();
    void on_pushButtonSaveSettings_clicked();
    void on_pushButtonInfo_clicked();

    // Start and stop communication
    void on_pushButtonStart_clicked();
    void on_pushButtonStop_clicked();

    void message(unsigned int id,QByteArray data);

    void on_pushButtonSend_clicked();

    void on_checkBoxActive1_stateChanged(int arg1);

    void on_checkBoxActive2_stateChanged(int arg1);

private:
    Ui::Dialog *ui;

    DLTCan dltCan;
    DLTMiniServer dltMiniServer;

    // Settings
    void restoreSettings();
    void updateSettings();

    int msgCounter;

};
#endif // DIALOG_H
