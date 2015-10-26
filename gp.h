//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2015

#ifndef GP_H
#define GP_H

#include "gp_global.h"
#include <QObject>
#include <QHash>
#include <QMutex>
#include <QDateTime>
#include <QAbstractSocket>
#include <QString>

// #define GP_WITH_STAT

#define GP_EALREADYLOGGEDIN      -1
#define GP_EINVALIDLOGINPARAMS   -2
#define GP_ENOSERVER             -3

#define GP_VERSION 0x010000
#define GP_MAGIC   0x010000
#define GP_HEADER_SIZE 8
#define GP_DEFAULT_PORT 6200
#define GP_TYPE_SYSTEM 0

#define GP_CMD_HELLO                        "HELLO"
#define GP_CMD_SERVER                       "SERVER"
#define GP_CMD_NETWORK_INFO                 "NETWORK_INFO"
#define GP_CMD_LOGIN                        "LOGIN"
#define GP_CMD_LOGIN_OK                     "LOGIN_OK"
#define GP_CMD_LOGIN_FAIL                   "LOGIN_FAIL"
#define GP_CMD_RAW                          "RAW"
#define GP_CMD_PERMDENY                     "PERMDENY"
#define GP_CMD_UNKNOWN                      "UNKNOWN"
#define GP_CMD_ERROR                        "ERROR"
//! This command is delivered when a new channel is joined by user
#define GP_CMD_CHANNEL_JOIN                 "CHANNEL_JOIN"
#define GP_CMD_CHANNEL_RESYNC               "CHANNEL_RESYNC"
#define GP_CMD_SCROLLBACK_RESYNC            "SCROLLBACK_RESYNC"
#define GP_CMD_SCROLLBACK_LOAD_NEW_ITEM     "SCROLLBACK_LOAD_NEW_ITEM"

class QTcpSocket;

namespace libgp
{
    //! Grumpy protocol

    //! This class is able to handle server or client connection using
    //! grumpy's network protocol
    class GPSHARED_EXPORT GP : public QObject
    {
            Q_OBJECT
        public:
            GP(QTcpSocket *tcp_socket = 0);
            virtual ~GP();
            virtual bool IsConnected() const;
            virtual void SendPacket(QHash<QString, QVariant> packet);
            virtual void SendProtocolCommand(QString command);
            virtual void SendProtocolCommand(QString command, QHash<QString, QVariant> parameters);
            //! Perform connection of Qt signals to internal functions,
            //! use this only if you aren't overriding this class
            virtual void ResolveSignals();
            virtual void Disconnect();
            unsigned long long GetBytesSent();
            unsigned long long GetBytesRcvd();
            virtual int GetVersion();
            unsigned long MaxIncomingCacheSize;

        signals:
            void Event_Connected();
            void Event_Disconnected();
            void Event_Timeout();
            void Event_Incoming(QHash<QString, QVariant> packet);
            void Event_IncomingCommand(QString text, QHash<QString, QVariant> parameters);

        public slots:
            virtual void OnPing();
            virtual void OnPingSend();
            virtual void OnError(QAbstractSocket::SocketError er);
            virtual void OnReceive();
            virtual void OnConnected();

        protected:
            virtual void OnIncomingCommand(QString text, QHash<QString, QVariant> parameters);
            void processPacket(QHash<QString, QVariant> pack);
            void processIncoming(QByteArray data);
            QHash<QString, QVariant> packetFromIncomingCache();
            void processHeader(QByteArray data);
            QMutex mutex;
            qint64 incomingPacketSize;
            QByteArray incomingCache;
            QTcpSocket *socket;

        private:
#ifdef GP_WITH_STAT
            unsigned long long largePacketSize;
            unsigned long long largestPacketSize;
            QDateTime currentPacketTime;
#endif
            unsigned long long sentBytes;
            unsigned long long recvBytes;
    };
}

#endif // GP_H

