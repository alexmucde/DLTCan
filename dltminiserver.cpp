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

#include "dltminiserver.h"

#include <QDebug>
#include <QFile>

DLTMiniServer::DLTMiniServer(QObject *parent) : QObject(parent)
{
    clearSettings();

    tcpSocket = 0;

}

DLTMiniServer::~DLTMiniServer()
{
    stop();
}

void DLTMiniServer::start()
{
    if(tcpServer.isListening())
        return;

    tcpServer.setMaxPendingConnections(1);
    if(tcpServer.listen(QHostAddress::Any,port)==true)
    {
        connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

        status("listening");
        qDebug() << "DLTMiniServer: listening" << port;
    }
    else
    {
        status("error");

        qDebug() << "DLTMiniServer: errorg" << port;
    }
}

void DLTMiniServer::stop()
{
    if(tcpSocket && tcpSocket->isOpen())
    {
        disconnect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
        disconnect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
        tcpSocket->close();
    }

    disconnect(&tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    tcpServer.close();

    status("stopped");
    qDebug() << "DLTMiniServer: stopped" << port;
}

void DLTMiniServer::clearSettings()
{
    port = 3491;
    applicationId = "DLT";
    contextId = "Mini";
}

void DLTMiniServer::writeSettings(QXmlStreamWriter &xml)
{
    /* Write project settings */
    xml.writeStartElement("DLTMiniServer");
        xml.writeTextElement("port",QString("%1").arg(port));
        xml.writeTextElement("applicationId",applicationId);
        xml.writeTextElement("contextId",contextId);
    xml.writeEndElement(); // DLTMiniServer
}

void DLTMiniServer::readSettings(const QString &filename)
{
    bool isDLTRelais = false;

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
             return;

    QXmlStreamReader xml(&file);

    while (!xml.atEnd())
    {
          xml.readNext();

          if(xml.isStartElement())
          {
              if(isDLTRelais)
              {
                  /* Project settings */
                  if(xml.name() == QString("port"))
                  {
                      port = xml.readElementText().toUShort();
                  }
                  if(xml.name() == QString("applicationId"))
                  {
                      applicationId = xml.readElementText();
                  }
                  if(xml.name() == QString("contextId"))
                  {
                      contextId = xml.readElementText();
                  }
              }
              else if(xml.name() == QString("DLTMiniServer"))
              {
                    isDLTRelais = true;
              }
          }
          else if(xml.isEndElement())
          {
              /* Connection, plugin and filter */
              if(xml.name() == QString("DLTMiniServer"))
              {
                    isDLTRelais = false;
              }
          }
    }
    if (xml.hasError())
    {
         qDebug() << "Error in processing filter file" << filename << xml.errorString();
    }

    file.close();


}

void DLTMiniServer::readyRead()
{

}

void DLTMiniServer::newConnection()
{
    tcpSocket = tcpServer.nextPendingConnection();
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    tcpServer.pauseAccepting();

    status("connected");
}

void DLTMiniServer::connected()
{

}

void DLTMiniServer::disconnected()
{
    tcpSocket->close();
    disconnect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    //delete tcpSocket;
    tcpSocket = 0;
    tcpServer.resumeAccepting();

    status("listening");
}

void DLTMiniServer::sendValue(QString text)
{
    if(tcpSocket==0 || !tcpSocket->isOpen())
    {
        return;
    }

    QByteArray data;

    // Standard Header (4 Byte)
    data += 0x21; // htyp: Use extended header, version 0x1
    data += (char)0x00; // message counter
    data += (char)0x00; // length high byte
    data += (char)4+10+4+2+text.length(); // length low byte

    // Extended Header (10 Byte)
    data += (char)0x41; // MSIN: Verbose,NW_TRACE, CAN 0x25
    data += (char)0x01; // NOAR
    data += applicationId[0].toLatin1(); // APID
    data += applicationId[1].toLatin1(); // APID
    data += applicationId[2].toLatin1(); // APID
    data += applicationId[3].toLatin1(); // APID
    data += contextId[0].toLatin1(); // CTID
    data += contextId[1].toLatin1(); // CTID
    data += contextId[2].toLatin1(); // CTID
    data += contextId[3].toLatin1(); // CTID

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text.toUtf8();

    tcpSocket->write(data);
}

void DLTMiniServer::sendValue2(QString text1,QString text2)
{
    if(tcpSocket==0 || !tcpSocket->isOpen())
    {
        return;
    }

    QByteArray data;

    // Standard Header (4 Byte)
    data += 0x21; // htyp: Use extended header, version 0x1
    data += (char)0x00; // message counter
    data += (char)0x00; // length high byte
    data += (char)4+10+4+2+text1.length()+4+2+text2.length(); // length low byte

    // Extended Header (10 Byte)
    data += (char)0x41; // MSIN: Verbose,NW_TRACE, CAN 0x25
    data += (char)0x02; // NOAR
    data += applicationId[0].toLatin1(); // APID
    data += applicationId[1].toLatin1(); // APID
    data += applicationId[2].toLatin1(); // APID
    data += applicationId[3].toLatin1(); // APID
    data += contextId[0].toLatin1(); // CTID
    data += contextId[1].toLatin1(); // CTID
    data += contextId[2].toLatin1(); // CTID
    data += contextId[3].toLatin1(); // CTID

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text1.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text1.toUtf8();

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text2.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text2.toUtf8();

    tcpSocket->write(data);

}

void DLTMiniServer::sendValue3(QString text1,QString text2,QString text3)
{
    if(tcpSocket==0 || !tcpSocket->isOpen())
    {
        return;
    }

    QByteArray data;

    // Standard Header (4 Byte)
    data += 0x21; // htyp: Use extended header, version 0x1
    data += (char)0x00; // message counter
    data += (char)0x00; // length high byte
    data += (char)4+10+4+2+text1.length()+4+2+text2.length()+4+2+text3.length(); // length low byte

    // Extended Header (10 Byte)
    data += (char)0x41; // MSIN: Verbose,NW_TRACE, CAN 0x25
    data += (char)0x03; // NOAR
    data += applicationId[0].toLatin1(); // APID
    data += applicationId[1].toLatin1(); // APID
    data += applicationId[2].toLatin1(); // APID
    data += applicationId[3].toLatin1(); // APID
    data += contextId[0].toLatin1(); // CTID
    data += contextId[1].toLatin1(); // CTID
    data += contextId[2].toLatin1(); // CTID
    data += contextId[3].toLatin1(); // CTID

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text1.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text1.toUtf8();

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text2.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text2.toUtf8();

    // Payload Type Info (4 Byte)
    data += (char)0x00;
    data += (char)0x02; // String
    data += (char)0x00;
    data += (char)0x00;

    // Payload Type Data Length
    data += ((char)text3.length()); // length low byte
    data += ((char)0x00); // length high byte

    // Payload Type Data
    data += text3.toUtf8();

    tcpSocket->write(data);

}
