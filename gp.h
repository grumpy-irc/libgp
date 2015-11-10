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
#include <QSslError>
#include <QMutex>
#include <QDateTime>
#include <QAbstractSocket>
#include <QString>

// #define GP_WITH_STAT

typedef unsigned int gp_command_t;

#define GP_INIT_DS(stream) stream.setVersion(QDataStream::Qt_4_0)

#define GP_ERROR                  500
#define GP_EALREADYLOGGEDIN      -1
#define GP_EINVALIDLOGINPARAMS   -2
#define GP_ENOSERVER             -3
#define GP_ENETWORKNOTFOUND      -4
#define GP_ESCROLLBACKNOTFOUND   -5
#define GP_ESSLHANDSHAKEFAILED   -20

#define GP_VERSION          0x010000
#define GP_MAGIC            0x010000
#define GP_HEADER_SIZE      8
#define GP_DEFAULT_PORT     6200
#define GP_DEFAULT_SSL_PORT 6208
#define GP_TYPE_SYSTEM      0

#define GP_CMD_HELLO                        1 //"HELLO"
#define GP_CMD_SERVER                       2 //"SERVER"
#define GP_CMD_NETWORK_INFO                 3 //"NETWORK_INFO"
//! When nickname was changed this requires update of network structure
#define GP_CMD_NICK                         4 //"NICK"
#define GP_CMD_LOGIN                        5 //"LOGIN"
#define GP_CMD_LOGIN_OK                     6 //"LOGIN_OK"
#define GP_CMD_LOGIN_FAIL                   7 //"LOGIN_FAIL"
#define GP_CMD_RAW                          8 //"RAW"
#define GP_CMD_PERMDENY                     9 //"PERMDENY"
#define GP_CMD_UNKNOWN                     10 //"UNKNOWN"
#define GP_CMD_MESSAGE                     11 //"MESSAGE"
#define GP_CMD_ERROR                       12 //"ERROR"
#define GP_CMD_IRC_QUIT                    13 //"IRC_QUIT"
//! This command is delivered when a new channel is joined by user
#define GP_CMD_CHANNEL_JOIN                14 //"CHANNEL_JOIN"
#define GP_CMD_CHANNEL_RESYNC              15 //"CHANNEL_RESYNC"
//! On resync of a whole network, this only involves internal network parameters
//! not channel lists and other lists of structures that have pointers
#define GP_CMD_NETWORK_RESYNC              16 //"NETWORK_RESYNC"
#define GP_CMD_SCROLLBACK_RESYNC           17 //"SCROLLBACK_RESYNC"
//! Used to save traffic on events where we need to resync some of the scrollback attributes but not buffer contents
//! only resync some of the scrollback items
#define GP_CMD_SCROLLBACK_PARTIAL_RESYNC   18 //"SCROLLBACK_PARTIAL_RESYNC"
#define GP_CMD_SCROLLBACK_LOAD_NEW_ITEM    19 //"SCROLLBACK_LOAD_NEW_ITEM"
#define GP_CMD_OPTIONS                     20 //"OPTIONS"
#define GP_CMD_USERLIST_SYNC               21 //"USERLIST_SYNC"
#define GP_CMD_REQUEST_ITEMS               22 //"REQUEST_ITEMS"

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
            virtual void Connect(QString host, int port, bool ssl);
            virtual bool IsConnected() const;
            virtual bool SendPacket(QHash<QString, QVariant> packet);
            virtual void SendProtocolCommand(unsigned int command);
            virtual void SendProtocolCommand(unsigned int command, QHash<QString, QVariant> parameters);
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
            void Event_ConnectionFailed(QString reason, int ec);
            void Event_Incoming(QHash<QString, QVariant> packet);
            void Event_SslHandshakeFailure(QList<QSslError> el, bool *is_ok);
            void Event_IncomingCommand(gp_command_t text, QHash<QString, QVariant> parameters);

        protected slots:
            virtual void OnPing();
            virtual void OnPingSend();
            virtual void OnError(QAbstractSocket::SocketError er);
            virtual void OnReceive();
            virtual void OnSslHandshakeFailure(QList<QSslError> el);
            virtual void OnConnected();
            virtual void OnDisconnect();

        protected:
            virtual void OnIncomingCommand(gp_command_t text, QHash<QString, QVariant> parameters);
            virtual void processPacket();
            virtual void processPacket(QHash<QString, QVariant> pack);
            virtual void processIncoming(QByteArray data);
            virtual void closeError(QString error, int code);
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
            bool isSSL;
            unsigned long long sentBytes;
            unsigned long long recvBytes;
            Thread *thread;
            bool isMultithreaded;
    };
}

#endif // GP_H

