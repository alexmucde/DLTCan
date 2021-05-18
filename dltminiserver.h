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
 * \file dltminiserver.h
 * @licence end@
 */

#ifndef DLTMINISERVER_H
#define DLTMINISERVER_H

#include <QObject>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QTcpServer>
#include <QTcpSocket>

class DLTMiniServer : public QObject
{
    Q_OBJECT
public:
    explicit DLTMiniServer(QObject *parent = nullptr);
    ~DLTMiniServer();

    void start();
    void stop();

    void sendValue(QString text);
    void sendValue2(QString text1,QString text2);
    void sendValue3(QString text1,QString text2,QString text3);

    unsigned short getPort() { return port; }
    void setPort(unsigned short port) { this->port = port; }

    QString getApplicationId() { return applicationId; }
    void setApplicationId(QString id) { this->applicationId = id; }

    QString getContextId() { return contextId; }
    void setContextId(QString id) { this->contextId = id; }

    void clearSettings();
    void writeSettings(QXmlStreamWriter &xml);
    void readSettings(const QString &filename);

signals:

    void status(QString text);
    void injection(QString text);

private slots:

    void readyRead();
    void newConnection();
    void connected();
    void disconnected();

private:

    QTcpServer tcpServer;
    QTcpSocket *tcpSocket;

    unsigned short port;
    QString applicationId;
    QString contextId;

    QByteArray readData;

};

#endif // DLTMINISERVER_H
