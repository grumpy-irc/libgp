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

#define GP_INIT_DS(stream) stream.setVersion(QDataStream::Qt_4_0)

#define GP_EALREADYLOGGEDIN      -1
#define GP_EINVALIDLOGINPARAMS   -2
#define GP_ENOSERVER             -3
#define GP_ENETWORKNOTFOUND      -4
#define GP_ESCROLLBACKNOTFOUND   -5

#define GP_VERSION          0x010000
#define GP_MAGIC            0x010000
#define GP_HEADER_SIZE      8
#define GP_DEFAULT_PORT     6200
#define GP_DEFAULT_SSL_PORT 6208
#define GP_TYPE_SYSTEM      0

#define GP_CMD_HELLO                        "HELLO"
#define GP_CMD_SERVER                       "SERVER"
#define GP_CMD_NETWORK_INFO                 "NETWORK_INFO"
//! When nickname was changed this requires update of network structure
#define GP_CMD_NICK                         "NICK"
#define GP_CMD_LOGIN                        "LOGIN"
#define GP_CMD_LOGIN_OK                     "LOGIN_OK"
#define GP_CMD_LOGIN_FAIL                   "LOGIN_FAIL"
#define GP_CMD_RAW                          "RAW"
#define GP_CMD_PERMDENY                     "PERMDENY"
#define GP_CMD_UNKNOWN                      "UNKNOWN"
#define GP_CMD_MESSAGE                      "MESSAGE"
#define GP_CMD_ERROR                        "ERROR"
#define GP_CMD_IRC_QUIT                     "IRC_QUIT"
//! This command is delivered when a new channel is joined by user
#define GP_CMD_CHANNEL_JOIN                 "CHANNEL_JOIN"
#define GP_CMD_CHANNEL_RESYNC               "CHANNEL_RESYNC"
//! On resync of a whole network, this only involves internal network parameters
//! not channel lists and other lists of structures that have pointers
#define GP_CMD_NETWORK_RESYNC               "NETWORK_RESYNC"
#define GP_CMD_SCROLLBACK_RESYNC            "SCROLLBACK_RESYNC"
//! Used to save traffic on events where we need to resync some of the scrollback attributes but not buffer contents
//! only resync some of the scrollback items
#define GP_CMD_SCROLLBACK_PARTIAL_RESYNC    "SCROLLBACK_PARTIAL_RESYNC"
#define GP_CMD_SCROLLBACK_LOAD_NEW_ITEM     "SCROLLBACK_LOAD_NEW_ITEM"

class QTcpSocket;

namespace libgp
{
    class Thread;

    //! Grumpy protocol

    //! This class is able to handle server or client connection using
    //! grumpy's network protocol
    class GPSHARED_EXPORT GP : public QObject
    {
            Q_OBJECT
        public:
            GP(QTcpSocket *tcp_socket = 0, bool mt = false);
            virtual ~GP();
            virtual bool IsConnected() const;
            virtual bool SendPacket(QHash<QString, QVariant> packet);
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
            friend class libgp::Thread;

        signals:
            void Event_Connected();
            void Event_Disconnected();
            void Event_Timeout();
            void Event_Incoming(QHash<QString, QVariant> packet);
            void Event_IncomingCommand(QString text, QHash<QString, QVariant> parameters);

        protected slots:
            virtual void OnPing();
            virtual void OnPingSend();
            virtual void OnError(QAbstractSocket::SocketError er);
            virtual void OnReceive();
            virtual void OnConnected();
            virtual void OnDisconnect();

        protected:
            virtual void OnIncomingCommand(QString text, QHash<QString, QVariant> parameters);
            virtual void processPacket();
            virtual void processPacket(QHash<QString, QVariant> pack);
            virtual void processIncoming(QByteArray data);
            QHash<QString, QVariant> packetFromIncomingCache();
            QHash<QString, QVariant> packetFromRawBytes(QByteArray packet);
            void processHeader(QByteArray data);
            QByteArray mtPop();
            QMutex mtLock;
            QList<QByteArray> mtBuffer;
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
            Thread *thread;
            bool isMultithreaded;
    };
}

#endif // GP_H

