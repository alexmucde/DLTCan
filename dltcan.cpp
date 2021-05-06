/**
 * @licence app begin@
 * Copyright (C) 2021 Alexander Wenzel
 *
 * This file is part of the DLT Multimeter project.
 *
 * \copyright This code is licensed under GPLv3.
 *
 * \author Alexander Wenzel <alex@eli2.de>
 *
 * \file dltcan.cpp
 * @licence end@
 */

#include "dltcan.h"

#include <QDebug>
#include <QFile>
#include <QSerialPortInfo>

DLTCan::DLTCan(QObject *parent) : QObject(parent)
{
    clearSettings();
}

DLTCan::~DLTCan()
{
    stop();
}

void DLTCan::checkPortName()
{
    /* check if information is stored, if not do not check */
    if(interfaceSerialNumber.isEmpty() && interfaceProductIdentifier==0 && interfaceVendorIdentifier==0)
        return;

    /* check if port information still matches port name */
    if((QSerialPortInfo(interface).serialNumber()!=interfaceSerialNumber ||
       QSerialPortInfo(interface).productIdentifier()!=interfaceProductIdentifier ||
       QSerialPortInfo(interface).vendorIdentifier()!=interfaceVendorIdentifier))
    {
        qDebug() << "Port" << interface << "not found anymore";

        /* port name has changed, try to find new port name */
        QList<QSerialPortInfo> 	availablePorts  = QSerialPortInfo::availablePorts();
        for(int num = 0; num<availablePorts.length();num++)
        {
            if(availablePorts[num].serialNumber()==interfaceSerialNumber &&
               availablePorts[num].productIdentifier()==interfaceProductIdentifier &&
               availablePorts[num].vendorIdentifier()==interfaceVendorIdentifier)
            {
                qDebug() << "Port name has changed from" << interface << "to" << availablePorts[num].portName();
                interface = availablePorts[num].portName();
            }
        }
    }
}

void DLTCan::start()
{
    if(!active)
    {
        status(QString("not active"));
        return;
    }

    // start communication
    checkPortName();

    // set serial port parameters
    serialPort.setBaudRate(QSerialPort::Baud115200);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setPortName(interface);

    // open serial port
    if(serialPort.open(QIODevice::ReadWrite)==true)
    {
        // open with success

        // connect slot to receive data from serial port
        connect(&serialPort, SIGNAL(readyRead()), this, SLOT(readyRead()));

        status(QString("started"));
        qDebug() << "DLTCan: started" << interface;
     }
    else
    {
        // open failed

        qDebug() << "DLTCan: Failed to open interface" << interface;
        status(QString("error"));
    }

    serialData.clear();

    // connect slot watchdog timer and start watchdog timer
    connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
    timer.start(5000);
    watchDogCounter = 0;
    watchDogCounterLast = 0;
    startFound = false;
}

void DLTCan::stop()
{
    if(!active)
    {
        return;
    }

    // stop communication
    status(QString("stopped"));
    qDebug() << "DLTCan: stopped" << interface;

    // close serial port, if it is open
    if(serialPort.isOpen())
    {
        serialPort.close();

        // disconnect slot to receive data from serial port
        disconnect(&serialPort, SIGNAL(readyRead()), this, SLOT(readyRead()));
    }

    // stop watchdog timer
    timer.stop();
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));

}

void DLTCan::readyRead()
{
    QByteArray data = serialPort.readAll();
    qDebug() << "DLTCan: Received " << data.toHex();
    for(int num=0;num<data.length();num++)
    {
       if(data.at(num)==0x7f)
       {
            if(startFound)
            {
                // already last Byte was start byte; remove duplicate
                rawData+=data[num];
                startFound = false;
            }
            else
            {
                 startFound = true;
            }
       }
       else
       {
           if(startFound)
           {
               // a new message starts
               rawData.clear();
           }
           rawData+=data[num];
           startFound = false;
       }

       // check if it is already a complete message
       if(rawData.size()==1 && (unsigned char)rawData.at(0)==0x01)
       {
           // send ok
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Send ok";
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0x02)
       {
           // send ok
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Watchdog";
           watchDogCounter++;
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0xfe)
       {
           // error send
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Send error";
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0x00)
       {
           // init ok
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Init ok";
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0xff)
       {
           // init error
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Init Error";
           rawData.clear();
       }
       else if(rawData.size()>=1 && (unsigned char)rawData.at(0)==0x80)
       {
           // standard CAN message
           if(rawData.size()>=4)
           {
               unsigned char length = rawData.at(1);
               if(rawData.size()>=(4+length))
               {
                   qDebug() << "DLTCan: Raw Data " << rawData.toHex();
                   unsigned short id = ((unsigned short)rawData.at(2)<<8)|((unsigned short)rawData.at(3));
                   QByteArray data = rawData.mid(4,length);
                   qDebug() << "DLTCan: Standard CAN message " << id << length << data.toHex();
                   message(id,data);
                   rawData.clear();
               }
           }
       }
       else if(rawData.size()>=1 && (unsigned char)rawData.at(0)==0x81)
       {
           // extended CAN message

           if(rawData.size()>=6)
           {
               qDebug() << "DLTCan: Raw Data " << rawData.toHex();
               unsigned char length = rawData.at(1);
               if(rawData.size()>=(6+length))
               {
                   unsigned int id = ((unsigned int)rawData.at(2)<<8)|((unsigned int)rawData.at(3)<<8)|((unsigned int)rawData.at(4)<<8)|((unsigned int)rawData.at(5));
                   QByteArray data = rawData.mid(6,length);
                   qDebug() << "DLTCan: Extended CAN message " << id << length << data.toHex();
                   message(id,data);
                   rawData.clear();
               }
           }
       }

    }
}

void DLTCan::timeout()
{
    // watchdog timeout

    // check if watchdog was triggered between last call
    if(watchDogCounter!=watchDogCounterLast)
    {
        watchDogCounterLast = watchDogCounter;
        status(QString("started"));
    }
    else
    {
        // no watchdog was received
        qDebug() << "DLTCan: Watchdog expired try to reconnect" ;

        // if serial port is open close serial port
        if(serialPort.isOpen())
        {
            serialPort.close();
            disconnect(&serialPort, SIGNAL(readyRead()), this, SLOT(readyRead()));
        }

        // check if port name has changed
        checkPortName();

        serialData.clear();

        // try to reopen serial port
        if(serialPort.open(QIODevice::ReadWrite)==true)
        {
            // retry was succesful

            // connect slot to receive data from serial port
            connect(&serialPort, SIGNAL(readyRead()), this, SLOT(readyRead()));

            status(QString("reconnect"));
            qDebug() << "DLTCan: reconnect" << interface;
        }
        else
        {
            // retry failed

            qDebug() << "DLTCan: Failed to open interface" << interface;
            status(QString("error"));
        }
    }

}

void DLTCan::clearSettings()
{
    active = 0;

    interfaceSerialNumber = "";
    interfaceProductIdentifier = 0;
    interfaceVendorIdentifier = 0;
}

void DLTCan::writeSettings(QXmlStreamWriter &xml)
{
    /* Write project settings */
    xml.writeStartElement(QString("DLTCan"));
        xml.writeTextElement("interface",interface);
        xml.writeTextElement("interfaceSerialNumber",QSerialPortInfo(interface).serialNumber());
        xml.writeTextElement("interfaceProductIdentifier",QString("%1").arg(QSerialPortInfo(interface).productIdentifier()));
        xml.writeTextElement("interfaceVendorIdentifier",QString("%1").arg(QSerialPortInfo(interface).vendorIdentifier()));
        xml.writeTextElement("active",QString("%1").arg(active));
    xml.writeEndElement(); // DLTCan
}

void DLTCan::readSettings(const QString &filename)
{
    bool isDLTCan = false;

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
             return;

    QXmlStreamReader xml(&file);

    while (!xml.atEnd())
    {
          xml.readNext();

          if(xml.isStartElement())
          {
              if(isDLTCan)
              {
                  /* Project settings */
                  if(xml.name() == QString("interface"))
                  {
                      interface = xml.readElementText();
                  }
                  else if(xml.name() == QString("interfaceSerialNumber"))
                  {
                      interfaceSerialNumber = xml.readElementText();
                  }
                  else if(xml.name() == QString("interfaceProductIdentifier"))
                  {
                      interfaceProductIdentifier = xml.readElementText().toUShort();
                  }
                  else if(xml.name() == QString("interfaceVendorIdentifier"))
                  {
                      interfaceVendorIdentifier = xml.readElementText().toUShort();
                  }
                  else if(xml.name() == QString("interfaceSerialNumber"))
                  {
                      interfaceSerialNumber = xml.readElementText();
                  }
                  else if(xml.name() == QString("interfaceProductIdentifier"))
                  {
                      interfaceProductIdentifier = xml.readElementText().toUShort();
                  }
                  else if(xml.name() == QString("interfaceVendorIdentifier"))
                  {
                      interfaceVendorIdentifier = xml.readElementText().toUShort();
                  }
                  else if(xml.name() == QString("active"))
                  {
                      active = xml.readElementText().toInt();
                  }
              }
              else if(xml.name() == QString("DLTCan"))
              {
                    isDLTCan = true;
              }
          }
          else if(xml.isEndElement())
          {
              /* Connection, plugin and filter */
              if(xml.name() == QString("DLTCan"))
              {
                    isDLTCan = false;
              }
          }
    }
    if (xml.hasError())
    {
         qDebug() << "Error in processing filter file" << filename << xml.errorString();
    }

    file.close();
}

void DLTCan::sendMessage(unsigned short id,unsigned char *data,int length)
{
    if(!active)
    {
        return;
    }

    unsigned char msg[256];

    msg[0]=0x7f;
    msg[1]=0x80;
    msg[2]=length;
    msg[3]=(id>>8)&0xff;
    msg[4]=id&0xff;
    memcpy((void*)(msg+5),(void*)data,length);
    serialPort.write((char*)msg,length+5);

    qDebug() << "DLTCan: Send CAN message " << id << length << QByteArray((char*)msg,length+5).toHex();

}
