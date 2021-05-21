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

#define DLT_LOG_FATAL 0x1
#define DLT_LOG_ERROR 0x2
#define DLT_LOG_WARN 0x3
#define DLT_LOG_INFO 0x4
#define DLT_LOG_DEBUG 0x5
#define DLT_LOG_VERBOSE 0x6

class DLTMiniServer : public QObject
{
    Q_OBJECT
public:
    explicit DLTMiniServer(QObject *parent = nullptr);
    ~DLTMiniServer();

    void start();
    void stop();

    void sendValue(QString text,int logLevel = DLT_LOG_INFO) { sendValue(applicationId,contextId,text,logLevel); }
    void sendValue2(QString text1,QString text2,int logLevel = DLT_LOG_INFO) { sendValue2(applicationId,contextId,text1,text2,logLevel); }
    void sendValue3(QString text1,QString text2,QString text3,int logLevel = DLT_LOG_INFO) { sendValue3(applicationId,contextId,text1,text2,text3,logLevel); }
    void sendValue(QString appId,QString ctxId, QString text,int logLevel = DLT_LOG_INFO);
    void sendValue2(QString appId,QString ctxId, QString text1,QString text2,int logLevel = DLT_LOG_INFO);
    void sendValue3(QString appId,QString ctxId, QString text1,QString text2,QString text3,int logLevel = DLT_LOG_INFO);

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
