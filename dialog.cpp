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
 * \file dialog.cpp
 * @licence end@
 */

#include <QSerialPortInfo>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

#include "dialog.h"
#include "ui_dialog.h"
#include "settingsdialog.h"
#include "version.h"

Dialog::Dialog(bool autostart,QString configuration,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , dltCan(this)
{
    ui->setupUi(this);

    // clear settings
    on_pushButtonDefaultSettings_clicked();

    // set window title with version information
    setWindowTitle(QString("DLTCan %1").arg(DLT_CAN_VERSION));
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    // disable stop button at startup
    ui->pushButtonStop->setDisabled(true);

    // connect status slots
    connect(&dltCan, SIGNAL(status(QString)), this, SLOT(statusCan(QString)));
    connect(&dltMiniServer, SIGNAL(status(QString)), this, SLOT(statusDlt(QString)));

    //  load global settings from registry
    QSettings settings;
    QString filename = settings.value("autoload/filename").toString();
    bool autoload = settings.value("autoload/checked").toBool();
    bool autostartGlobal = settings.value("autostart/checked").toBool();

    // autoload settings, when activated in global settings
    if(autoload)
    {
        dltCan.readSettings(filename);
        dltMiniServer.readSettings(filename);
        restoreSettings();
    }

    // autoload settings, when provided by command line
    if(!configuration.isEmpty())
    {
        dltCan.readSettings(configuration);
        dltMiniServer.readSettings(configuration);
        restoreSettings();
    }

    // autostart, when activated in global settings or by command line
    if(autostartGlobal || autostart)
    {
        on_pushButtonStart_clicked();
    }
}

Dialog::~Dialog()
{

    // disconnect all slots
    disconnect(&dltCan, SIGNAL(status(QString)), this, SLOT(statusCan(QString)));
    disconnect(&dltMiniServer, SIGNAL(status(QString)), this, SLOT(statusDlt(QString)));

    delete ui;
}

void Dialog::restoreSettings()
{

}

void Dialog::updateSettings()
{
}

void Dialog::on_pushButtonStart_clicked()
{
    // start communication
    updateSettings();

    // start Relais and DLT communication
    dltCan.start();
    dltMiniServer.start();

    // disable settings and start button
    // enable stop button
    ui->pushButtonStart->setDisabled(true);
    ui->pushButtonStop->setDisabled(false);
    ui->pushButtonDefaultSettings->setDisabled(true);
    ui->pushButtonLoadSettings->setDisabled(true);
    ui->pushButtonSettings->setDisabled(true);

    connect(&dltCan, SIGNAL(message(unsigned int,QByteArray)), this, SLOT(message(unsigned int,QByteArray)));

}

void Dialog::on_pushButtonStop_clicked()
{
    // stop communication

    disconnect(&dltCan, SIGNAL(message(unsigned int,QByteArray)), this, SLOT(message(unsigned int,QByteArray)));

    // stop Relais and DLT communication
    dltCan.stop();
    dltMiniServer.stop();

    // enable settings and start button
    // disable stop button
    ui->pushButtonStart->setDisabled(false);
    ui->pushButtonStop->setDisabled(true);
    ui->pushButtonDefaultSettings->setDisabled(false);
    ui->pushButtonLoadSettings->setDisabled(false);
    ui->pushButtonSettings->setDisabled(false);
}

void Dialog::statusCan(QString text)
{
    // status from Relais

    // status of CAN communication changed
    if(text == "" || text == "stopped" || text == "not active")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::white);
        ui->lineEditStatusCan->setPalette(palette);
        ui->lineEditStatusCan->setText(text);
    }
    else if(text == "started")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::green);
        ui->lineEditStatusCan->setPalette(palette);
        ui->lineEditStatusCan->setText(text);
    }
    else if(text == "reconnect")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::yellow);
        ui->lineEditStatusCan->setPalette(palette);
        ui->lineEditStatusCan->setText(text);
    }
    else if(text == "error")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::red);
        ui->lineEditStatusCan->setPalette(palette);
        ui->lineEditStatusCan->setText(text);
    }
}

void Dialog::statusDlt(QString text)
{
    // status from DLT Mini Server
    ui->lineEditStatusDLT->setText(text);

    // status of DLT communication changed
    if(text == "" || text == "stopped")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::white);
        ui->lineEditStatusDLT->setPalette(palette);
    }
    else if(text == "listening")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::yellow);
        ui->lineEditStatusDLT->setPalette(palette);
    }
    else if(text == "connected")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::green);
        ui->lineEditStatusDLT->setPalette(palette);
    }
    else if(text == "error")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::red);
        ui->lineEditStatusDLT->setPalette(palette);
    }
}

void Dialog::on_pushButtonDefaultSettings_clicked()
{
    // Reset settings to default
    dltCan.clearSettings();
    dltMiniServer.clearSettings();

    restoreSettings();
}

void Dialog::on_pushButtonLoadSettings_clicked()
{
    // Load settings from XML file

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Settings"), "", tr("DLTCan Settings (*.xml);;All files (*.*)"));

    if(fileName.isEmpty())
    {
        // No file was selected or cancel was pressed
        return;
    }

    // read the settings from XML file
    dltCan.readSettings(fileName);
    dltMiniServer.readSettings(fileName);

    restoreSettings();
}

void Dialog::on_pushButtonSaveSettings_clicked()
{
    // Save settings into XML file

    updateSettings();

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Settings"), "", tr("DLTCan Settings (*.xml);;All files (*.*)"));

    if(fileName.isEmpty())
    {
        // No file was selected or cancel was pressed
        return;
    }

    // read the settings from XML file
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        // Cannot open the file for writing
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);

    // FIXME: Cannot read data from XML file, which contains a start document
    // So currently do not call StartDocument
    //xml.writeStartDocument();

    xml.writeStartElement("DLTCanSettings");
        dltCan.writeSettings(xml);
        dltMiniServer.writeSettings(xml);
    xml.writeEndElement(); // DLTRelaisSettings

    // FIXME: Cannot read data from XML file, which contains a end document
    // So currently do not call EndDocument
    //xml.writeEndDocument();
    file.close();

}

void Dialog::on_pushButtonSettings_clicked()
{
    // Open settings dialog
    SettingsDialog dlg(this);

    dlg.restoreSettings(&dltCan, &dltMiniServer);
    if(dlg.exec()==QDialog::Accepted)
    {
        dlg.backupSettings(&dltCan, &dltMiniServer);
        restoreSettings();
    }
}

void Dialog::on_pushButtonInfo_clicked()
{
    // Open information window
    QMessageBox msgBox(this);

    msgBox.setWindowTitle("Info DLTPower");
    msgBox.setTextFormat(Qt::RichText);

    QString text;
    text += QString("Version: %1<br>").arg(DLT_CAN_VERSION);
    text += "<br>";
    text += "Information and Documentation can be found here:<br>";
    text += "<br>";
    text += "<a href='https://github.com/alexmucde/DLTCan'>Github DLTCan</a><br>";
    text += "<br>";
    text += "This SW is licensed under GPLv3.<br>";
    text += "<br>";
    text += "(C) 2021 Alexander Wenzel <alex@eli2.de>";

    msgBox.setText(text);

    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();
}

void  Dialog::message(unsigned int id,QByteArray data)
{
    dltMiniServer.sendValue3("CAN",QString("%1").arg(id, 8, 16, QLatin1Char( '0' )),data.toHex());
}
