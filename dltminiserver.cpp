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

    readData.clear();

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
    while(tcpSocket && tcpSocket->bytesAvailable())
    {
        readData += tcpSocket->readAll();

        // check if complete DLT message is received
        do
        {
            if(readData.size()>=4)
            {
                // calculate size
                unsigned short length = (unsigned short)(readData[3]) | ((unsigned short)(readData[2]) << 8);
                if(readData.size()>=length)
                {
                    qDebug() << "DLTMiniServer: msg received with length" << length;

                    unsigned char htyp = (unsigned char)(readData[0]);

                    unsigned short standardHeaderLength = 4;
                    if(htyp&0x04) standardHeaderLength+=4; // with ecu id
                    if(htyp&0x08) standardHeaderLength+=4; // with session id
                    if(htyp&0x10) standardHeaderLength+=4; // with timestamp

                    //qDebug() << "DLTMiniServer: header length" << standardHeaderLength;

                    if(htyp&0x01) // use of extended header
                    {
                        unsigned char msin = (unsigned char)(readData[standardHeaderLength]);
                        unsigned char mstp = (msin >> 1) & 0x07;
                        unsigned char mtin = (msin >> 4) & 0x0f;

                        //qDebug() << "DLTMiniServer: mstp" << mstp << "mtin" << mtin;

                        if(mstp==0x3 && mtin == 0x01 && readData.size()>=standardHeaderLength+10+4) // Control request message
                        {
                            unsigned int serviceId = (unsigned int)(readData[standardHeaderLength+10]) |
                                                     (unsigned int)(readData[standardHeaderLength+11]) << 8 |
                                                    (unsigned int)(readData[standardHeaderLength+12]) << 16 |
                                                    (unsigned int)(readData[standardHeaderLength+13]) << 24;

                            //qDebug() << "DLTMiniServer: serviceId" << serviceId;

                            if(serviceId==4096 && readData.size()>=standardHeaderLength+10+4+4)
                            {
                                unsigned int lengthData = (unsigned int)(readData[standardHeaderLength+14]) |
                                                            (unsigned int)(readData[standardHeaderLength+15]) << 8 |
                                                            (unsigned int)(readData[standardHeaderLength+15]) << 16 |
                                                            (unsigned int)(readData[standardHeaderLength+15]) << 24;

                                //qDebug() << "DLTMiniServer: lengthData" << lengthData;

                                if(readData.size()>=standardHeaderLength+18+lengthData)
                                {
                                    QByteArray injectionData = readData.mid(standardHeaderLength+18,lengthData);
                                    QString injectionStr = QString::fromLatin1(injectionData);

                                    qDebug() << "DLTMiniServer: injection" << injectionStr;

                                    injection(injectionStr);
                                }
                            }

                        }
                    }
                    readData.remove(0,length); // full message received, delete
                }
                else
                {
                    break; // no full message received
                }
            }
            else
            {
                break; // no full message received
            }
        } while(true);
    }
}

void DLTMiniServer::newConnection()
{
    tcpSocket = tcpServer.nextPendingConnection();
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    tcpServer.pauseAccepting();

    readData.clear();

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
    disconnect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    //delete tcpSocket;
    tcpSocket = 0;
    tcpServer.resumeAccepting();

    readData.clear();

    status("listening");
}

void DLTMiniServer::sendValue(QString appId,QString ctxId, QString text,int logLevel)
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
    data += (char)0x01|(char)logLevel<<4; // MSIN: Verbose,DLT_TYPE_LOG
    data += (char)0x01; // NOAR
    data += appId[0].toLatin1(); // APID
    data += appId[1].toLatin1(); // APID
    data += appId[2].toLatin1(); // APID
    data += appId[3].toLatin1(); // APID
    data += ctxId[0].toLatin1(); // CTID
    data += ctxId[1].toLatin1(); // CTID
    data += ctxId[2].toLatin1(); // CTID
    data += ctxId[3].toLatin1(); // CTID

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

void DLTMiniServer::sendValue2(QString appId,QString ctxId, QString text1,QString text2,int logLevel)
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
    data += (char)0x01|(char)logLevel<<4; // MSIN: Verbose,DLT_TYPE_LOG
    data += (char)0x02; // NOAR
    data += appId[0].toLatin1(); // APID
    data += appId[1].toLatin1(); // APID
    data += appId[2].toLatin1(); // APID
    data += appId[3].toLatin1(); // APID
    data += ctxId[0].toLatin1(); // CTID
    data += ctxId[1].toLatin1(); // CTID
    data += ctxId[2].toLatin1(); // CTID
    data += ctxId[3].toLatin1(); // CTID

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

void DLTMiniServer::sendValue3(QString appId,QString ctxId, QString text1,QString text2,QString text3,int logLevel)
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
    data += (char)0x01|(char)logLevel<<4; // MSIN: Verbose,DLT_TYPE_LOG
    data += (char)0x03; // NOAR
    data += appId[0].toLatin1(); // APID
    data += appId[1].toLatin1(); // APID
    data += appId[2].toLatin1(); // APID
    data += appId[3].toLatin1(); // APID
    data += ctxId[0].toLatin1(); // CTID
    data += ctxId[1].toLatin1(); // CTID
    data += ctxId[2].toLatin1(); // CTID
    data += ctxId[3].toLatin1(); // CTID

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
