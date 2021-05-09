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
 * \file dltcan.h
 * @licence end@
 */

#ifndef DLT_CAN_H
#define DLT_CAN_H

#include <QObject>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSerialPort>
#include <QTimer>

class DLTCan : public QObject
{
    Q_OBJECT
public:
    explicit DLTCan(QObject *parent = nullptr);
    ~DLTCan();

    void checkPortName();
    void start();
    void stop();

    QString getInterface() { return interface; }
    void setInterface(QString interface) { this->interface = interface; }

    // Active
    bool getActive() { return active; }
    void setActive(bool active) { this->active = active; }

    void clearSettings();
    void writeSettings(QXmlStreamWriter &xml);
    void readSettings(const QString &filename);

    void on();
    void off();

    void sendMessage(unsigned short id,unsigned char *data,int length);
    void startCyclicMessage1(int timeout);
    void startCyclicMessage2(int timeout);
    void setCyclicMessage1(unsigned short id,QByteArray data);
    void setCyclicMessage2(unsigned short id,QByteArray data);
    void stopCyclicMessage1();
    void stopCyclicMessage2();

    unsigned short getMessageId() const;
    void setMessageId(unsigned short value);

    unsigned short getCyclicMessageId1() const;
    void setCyclicMessageId1(unsigned short value);

    unsigned short getCyclicMessageId2() const;
    void setCyclicMessageId2(unsigned short value);

    QByteArray getMessageData() const;
    void setMessageData(const QByteArray &value);

    QByteArray getCyclicMessageData1() const;
    void setCyclicMessageData1(const QByteArray &value);

    QByteArray getCyclicMessageData2() const;
    void setCyclicMessageData2(const QByteArray &value);

    int getCyclicMessageTimeout1() const;
    void setCyclicMessageTimeout1(int value);

    int getCyclicMessageTimeout2() const;
    void setCyclicMessageTimeout2(int value);

    bool getCyclicMessageActive1() const;
    void setCyclicMessageActive1(bool value);

    bool getCyclicMessageActive2() const;
    void setCyclicMessageActive2(bool value);

signals:

    void status(QString text);
    void message(unsigned int id,QByteArray data);

private slots:

    void readyRead();

    // Watchdog Timeout
    void timeout();

    void timeoutCyclicMessage1();
    void timeoutCyclicMessage2();

private:

    QSerialPort serialPort;
    QTimer timer;
    QTimer timerRequest;
    unsigned int watchDogCounter,watchDogCounterLast;

    QString interface;
    QString interfaceSerialNumber;
    ushort interfaceProductIdentifier;
    ushort interfaceVendorIdentifier;
    bool active;

    QByteArray serialData;

    QByteArray rawData;
    bool startFound;

    bool cyclicMessageActive1,cyclicMessageActive2;
    int cyclicMessageTimeout1,cyclicMessageTimeout2;
    unsigned short messageId,cyclicMessageId1,cyclicMessageId2;
    QByteArray messageData,cyclicMessageData1,cyclicMessageData2;

    QTimer timerCyclicMessage1;
    QTimer timerCyclicMessage2;

};

#endif // DLT_CAN_H
