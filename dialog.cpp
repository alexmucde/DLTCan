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

    connect(&dltMiniServer, SIGNAL(injection(QString)), this, SLOT(injection(QString)));

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

    disconnect(&dltMiniServer, SIGNAL(injection(QString)), this, SLOT(injection(QString)));

    delete ui;
}

void Dialog::restoreSettings()
{
    ui->lineEditMsgId->setText(QString("%1").arg(dltCan.getMessageId(),0,16));
    ui->lineEditMsgData->setText(dltCan.getMessageData().toHex());
    ui->lineEditTime1->setText(QString("%1").arg(dltCan.getCyclicMessageTimeout1()));
    ui->lineEditMsgId1->setText(QString("%1").arg(dltCan.getCyclicMessageId1(),0,16));
    ui->lineEditMsgData1->setText(dltCan.getCyclicMessageData1().toHex());
    ui->lineEditTime2->setText(QString("%1").arg(dltCan.getCyclicMessageTimeout2()));
    ui->lineEditMsgId2->setText(QString("%1").arg(dltCan.getCyclicMessageId2(),0,16));
    ui->lineEditMsgData2->setText(dltCan.getCyclicMessageData2().toHex());
    ui->checkBoxActive1->setChecked(dltCan.getCyclicMessageActive1());
    ui->checkBoxActive2->setChecked(dltCan.getCyclicMessageActive2());
}

void Dialog::updateSettings()
{
    dltCan.setMessageId(ui->lineEditMsgId->text().toUShort(nullptr,16));
    dltCan.setMessageData(QByteArray::fromHex(ui->lineEditMsgData->text().toLatin1()));
    dltCan.setCyclicMessageActive1(ui->checkBoxActive1->isChecked());
    dltCan.setCyclicMessageTimeout1(ui->lineEditTime1->text().toInt());
    dltCan.setCyclicMessageId1(ui->lineEditMsgId1->text().toUShort(nullptr,16));
    dltCan.setCyclicMessageData1(QByteArray::fromHex(ui->lineEditMsgData1->text().toLatin1()));
    dltCan.setCyclicMessageActive2(ui->checkBoxActive2->isChecked());
    dltCan.setCyclicMessageTimeout2(ui->lineEditTime2->text().toInt());
    dltCan.setCyclicMessageId2(ui->lineEditMsgId2->text().toUShort(nullptr,16));
    dltCan.setCyclicMessageData2(QByteArray::fromHex(ui->lineEditMsgData2->text().toLatin1()));
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

    msgCounter = 0;
    ui->lineEditMsgCount->setText(QString("%1").arg(msgCounter));
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
    else if(text == "started" || text == "send ok" || text == "init ok")
    {
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::green);
        ui->lineEditStatusCan->setPalette(palette);
        ui->lineEditStatusCan->setText(text);
    }
    else if(text == "reconnect" || text=="send error" || text == "init error")
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

    msgCounter++;
    ui->lineEditMsgCount->setText(QString("%1").arg(msgCounter));
}

void Dialog::on_pushButtonSend_clicked()
{
    unsigned short id = ui->lineEditMsgId->text().toUShort(nullptr,16);
    QByteArray data = QByteArray::fromHex(ui->lineEditMsgData->text().toLatin1());
    dltCan.sendMessage(id,(unsigned char*)data.data(),data.length());
}

void Dialog::on_checkBoxActive1_stateChanged(int arg1)
{
    if(arg1)
    {
        dltCan.setCyclicMessage1(ui->lineEditMsgId1->text().toUShort(nullptr,16),QByteArray::fromHex(ui->lineEditMsgData1->text().toLatin1()));
        dltCan.startCyclicMessage1(ui->lineEditTime1->text().toInt());
    }
    else
    {dltCan.stopCyclicMessage1();

    }
}

void Dialog::on_checkBoxActive2_stateChanged(int arg1)
{
    if(arg1)
    {
        dltCan.setCyclicMessage2(ui->lineEditMsgId2->text().toUShort(nullptr,16),QByteArray::fromHex(ui->lineEditMsgData2->text().toLatin1()));
        dltCan.startCyclicMessage2(ui->lineEditTime2->text().toInt());
    }
    else
    {
        dltCan.stopCyclicMessage2();
    }

}

void Dialog::injection(QString text)
{
    QStringList list = text.split(' ');

    qDebug() << "Injection received: " << text;

    if(list[0] == "CAN")
    {
        unsigned short id = list[1].toUShort(nullptr,16);
        QByteArray data = QByteArray::fromHex(list[2].toLatin1());
        dltCan.sendMessage(id,(unsigned char*)data.data(),data.length());
    }
    else if(list[0] == "CANCYC1")
    {
        if(list[1]=="off")
        {
            ui->checkBoxActive1->setChecked(false);
            on_checkBoxActive1_stateChanged(false);
        }
        else
        {
            unsigned short time = list[1].toUShort();
            unsigned short id = list[2].toUShort(nullptr,16);
            QByteArray data = QByteArray::fromHex(list[3].toLatin1());

            dltCan.setCyclicMessage1(id,data);
            dltCan.startCyclicMessage1(time);

            restoreSettings();
        }
    }
    else if(list[0] == "CANCYC2")
    {
        if(list[1]=="off")
        {
            ui->checkBoxActive2->setChecked(false);
            on_checkBoxActive2_stateChanged(false);
        }
        else
        {
            unsigned short time = list[1].toUShort();
            unsigned short id = list[2].toUShort(nullptr,16);
            QByteArray data = QByteArray::fromHex(list[3].toLatin1());

            dltCan.setCyclicMessage2(id,data);
            dltCan.startCyclicMessage2(time);

            restoreSettings();
        }
    }

}
