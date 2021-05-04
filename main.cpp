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
 * \file main.h
 * @licence end@
 */

#include "dialog.h"
#include "version.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("alexmucde");
    QCoreApplication::setOrganizationDomain("github.com");
    QCoreApplication::setApplicationName("DLTCan");
    QCoreApplication::setApplicationVersion(DLT_CAN_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("CAN Datalogger for DLT.");
    const QCommandLineOption helpOption = parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("configuration", QCoreApplication::translate("main", "Configuration file."));

    // Option Autostart
    QCommandLineOption autostartOption("a", QCoreApplication::translate("main", "Autostart Communication"));
    parser.addOption(autostartOption);

    // Parse the Arguments
    parser.process(a);

    // Stop application if help is called
    if(parser.isSet(helpOption))
            return 1;

    // set command line options
    QString configuration;
    if(parser.positionalArguments().size()>=1)
        configuration = parser.positionalArguments().at(0);
    qDebug() << "Option: configuration =" << configuration;
    bool autostart= false;
    autostart = parser.isSet(autostartOption);
    qDebug() << "Option: -a =" << autostart;

    // execute dialog
    Dialog w(autostart,configuration);
    w.show();
    return a.exec();
}
