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
        status("not active");
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

        status("started");
        qDebug() << "DLTCan: started" << interface;
     }
    else
    {
        // open failed

        qDebug() << "DLTCan: Failed to open interface" << interface;
        status("error");
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
    status("stopped");
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
           status("send ok");
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
           status("send error");
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0x00)
       {
           // init ok
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Init ok";
           status("init ok");
           rawData.clear();
       }
       else if(rawData.size()==1 && (unsigned char)rawData.at(0)==0xff)
       {
           // init error
           qDebug() << "DLTCan: Raw Data " << rawData.toHex();
           qDebug() << "DLTCan: Init Error";
           status("init error");
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
        status("started");
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

            status("reconnect");
            qDebug() << "DLTCan: reconnect" << interface;
        }
        else
        {
            // retry failed

            qDebug() << "DLTCan: Failed to open interface" << interface;
            status("error");
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
        xml.writeTextElement("messageId",QString("%1").arg(messageId));
        xml.writeTextElement("messageData",messageData.toHex());
        xml.writeTextElement("cyclicMessageActive1",QString("%1").arg(cyclicMessageActive1));
        xml.writeTextElement("cyclicMessageTimeout1",QString("%1").arg(cyclicMessageTimeout1));
        xml.writeTextElement("cyclicMessageId1",QString("%1").arg(cyclicMessageId1));
        xml.writeTextElement("cyclicMessageData1",cyclicMessageData1.toHex());
        xml.writeTextElement("cyclicMessageActive2",QString("%1").arg(cyclicMessageActive2));
        xml.writeTextElement("cyclicMessageTimeout2",QString("%1").arg(cyclicMessageTimeout2));
        xml.writeTextElement("cyclicMessageId2",QString("%1").arg(cyclicMessageId2));
        xml.writeTextElement("cyclicMessageData2",cyclicMessageData2.toHex());
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
                  else if(xml.name() == QString("messageId"))
                  {
                      messageId = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("messageData"))
                  {
                      messageData = QByteArray::fromHex(xml.readElementText().toLatin1());
                  }
                  else if(xml.name() == QString("cyclicMessageActive1"))
                  {
                      cyclicMessageActive1 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageTimeout1"))
                  {
                      cyclicMessageTimeout1 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageId1"))
                  {
                      cyclicMessageId1 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageData1"))
                  {
                      cyclicMessageData1 = QByteArray::fromHex(xml.readElementText().toLatin1());
                  }
                  else if(xml.name() == QString("cyclicMessageActive2"))
                  {
                      cyclicMessageActive2 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageTimeout2"))
                  {
                      cyclicMessageTimeout2 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageId2"))
                  {
                      cyclicMessageId2 = xml.readElementText().toInt();
                  }
                  else if(xml.name() == QString("cyclicMessageData2"))
                  {
                      cyclicMessageData2 = QByteArray::fromHex(xml.readElementText().toLatin1());
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
    int pos = 5;
    for(int num=0;num<length;num++)
    {
        msg[pos++]=data[num];
        //if(data[num]==0x7f)
        //    msg[pos++]=0x7f; // add stuff byte to be able to detect unique header
    }
    //memcpy((void*)(msg+5),(void*)data,length);
    serialPort.write((char*)msg,pos);

    messageId = id;
    messageData = QByteArray((char*)data,length);

    qDebug() << "DLTCan: Send CAN message " << id << length << QByteArray((char*)msg,pos).toHex();

    message(id,QByteArray((char*)data,length));

}

void DLTCan::startCyclicMessage1(int timeout)
{
    connect(&timerCyclicMessage1, SIGNAL(timeout()), this, SLOT(timeoutCyclicMessage1()));

    cyclicMessageTimeout1 = timeout;
    cyclicMessageActive1 = true;

    timerCyclicMessage1.start(timeout);
}

void DLTCan::startCyclicMessage2(int timeout)
{
    connect(&timerCyclicMessage2, SIGNAL(timeout()), this, SLOT(timeoutCyclicMessage2()));

    cyclicMessageTimeout2 = timeout;
    cyclicMessageActive2 = true;

    timerCyclicMessage2.start(timeout);
}

void DLTCan::stopCyclicMessage1()
{
    timerCyclicMessage1.stop();
    cyclicMessageActive1 = false;

    disconnect(&timerCyclicMessage1, SIGNAL(timeout()), this, SLOT(timeoutCyclicMessage1()));
}

void DLTCan::stopCyclicMessage2()
{
    timerCyclicMessage2.stop();
    cyclicMessageActive2 = false;

    disconnect(&timerCyclicMessage2, SIGNAL(timeout()), this, SLOT(timeoutCyclicMessage2()));
}

void DLTCan::timeoutCyclicMessage1()
{
    if(!active)
    {
        return;
    }

    unsigned char msg[256];

    msg[0]=0x7f;
    msg[1]=0x80;
    msg[2]=cyclicMessageData1.length();
    msg[3]=(cyclicMessageId1>>8)&0xff;
    msg[4]=cyclicMessageId1&0xff;
    memcpy((void*)(msg+5),(void*)cyclicMessageData1.constData(),cyclicMessageData1.length());
    serialPort.write((char*)msg,cyclicMessageData1.length()+5);

    qDebug() << "DLTCan: Send CAN message " << cyclicMessageId1 << cyclicMessageData1.length() << QByteArray((char*)msg,cyclicMessageData1.length()+5).toHex();

    message(cyclicMessageId1,QByteArray((char*)cyclicMessageData1.constData(),cyclicMessageData1.length()));
}

void DLTCan::timeoutCyclicMessage2()
{
    if(!active)
    {
        return;
    }

    unsigned char msg[256];

    msg[0]=0x7f;
    msg[1]=0x80;
    msg[2]=cyclicMessageData2.length();
    msg[3]=(cyclicMessageId2>>8)&0xff;
    msg[4]=cyclicMessageId2&0xff;
    memcpy((void*)(msg+5),(void*)cyclicMessageData2.constData(),cyclicMessageData2.length());
    serialPort.write((char*)msg,cyclicMessageData2.length()+5);

    qDebug() << "DLTCan: Send CAN message " << cyclicMessageId2 << cyclicMessageData2.length() << QByteArray((char*)msg,cyclicMessageData2.length()+5).toHex();

    message(cyclicMessageId2,QByteArray((char*)cyclicMessageData2.constData(),cyclicMessageData2.length()));
}

bool DLTCan::getCyclicMessageActive2() const
{
    return cyclicMessageActive2;
}

void DLTCan::setCyclicMessageActive2(bool value)
{
    cyclicMessageActive2 = value;
}

bool DLTCan::getCyclicMessageActive1() const
{
    return cyclicMessageActive1;
}

void DLTCan::setCyclicMessageActive1(bool value)
{
    cyclicMessageActive1 = value;
}

int DLTCan::getCyclicMessageTimeout2() const
{
    return cyclicMessageTimeout2;
}

void DLTCan::setCyclicMessageTimeout2(int value)
{
    cyclicMessageTimeout2 = value;
}

int DLTCan::getCyclicMessageTimeout1() const
{
    return cyclicMessageTimeout1;
}

void DLTCan::setCyclicMessageTimeout1(int value)
{
    cyclicMessageTimeout1 = value;
}

QByteArray DLTCan::getCyclicMessageData2() const
{
    return cyclicMessageData2;
}

void DLTCan::setCyclicMessageData2(const QByteArray &value)
{
    cyclicMessageData2 = value;
}

QByteArray DLTCan::getCyclicMessageData1() const
{
    return cyclicMessageData1;
}

void DLTCan::setCyclicMessageData1(const QByteArray &value)
{
    cyclicMessageData1 = value;
}

QByteArray DLTCan::getMessageData() const
{
    return messageData;
}

void DLTCan::setMessageData(const QByteArray &value)
{
    messageData = value;
}

unsigned short DLTCan::getCyclicMessageId2() const
{
    return cyclicMessageId2;
}

void DLTCan::setCyclicMessageId2(unsigned short value)
{
    cyclicMessageId2 = value;
}

unsigned short DLTCan::getCyclicMessageId1() const
{
    return cyclicMessageId1;
}

void DLTCan::setCyclicMessageId1(unsigned short value)
{
    cyclicMessageId1 = value;
}

unsigned short DLTCan::getMessageId() const
{
    return messageId;
}

void DLTCan::setMessageId(unsigned short value)
{
    messageId = value;
}

void DLTCan::setCyclicMessage1(unsigned short id,QByteArray data)
{
    cyclicMessageId1 = id;
    cyclicMessageData1 = data;
}

void DLTCan::setCyclicMessage2(unsigned short id,QByteArray data)
{
    cyclicMessageId2 = id;
    cyclicMessageData2 = data;
}
