//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2015 - 2018

#ifndef GP_H
#define GP_H

#include "gp_global.h"
#include <QObject>
#include <QHash>
#include <QSslError>
#include <QDateTime>
#include <QAbstractSocket>
#include <QString>

// #define GP_WITH_STAT

typedef unsigned int gp_command_t;
typedef unsigned char gp_byte_t;

#define GP_INIT_DS(stream) stream.setVersion(QDataStream::Qt_4_0)

#define GP_PROTOCOL_SERIOUS_FAILURE  200
#define GP_ERROR                     500

#define GP_VERSION            0x010000
#define GP_MAGIC              0x010000
#define GP_HEADER_SIZE        5
#define GP_DEFAULT_PORT       6200
#define GP_DEFAULT_SSL_PORT   6208
#define GP_TYPE_SYSTEM        0
#define GP_TYPE_COMPRESSION   1
#define GP_TYPE_PING          2

class QTcpSocket;
class QMutex;
class QTimer;

namespace libgp
{
    class Thread;

    //! Grumpy protocol

    //! This class is able to handle server or client connection using
    //! grumpy's network protocol
    //!
    //! Visualisation of packet
    //! <        HEADER       >|<                     DATA                         >
    //! +--------+-------------+---------------------------------------------------+
    //! | SIZE   | COMPRESSION | Remaining data                                    |
    //! +--------+-------------+---------------------------------------------------+
    //!
    //! The size is 8 bytes long integer and compression 1 byte long,
    //! in total 9 bytes
    //!
    //! How this works:
    //! This protocol is in its nature extremely simple, it takes the input data in form
    //! of a QHash<QString, QVariable> (nested hashes supported), serialize them using
    //! byte stream, optinally compress them, and then it will deliver them to other side
    //! using the schema above.
    //!
    //! On other side, the GP protocol will deserialize data back to QHash<QString, QVariant>
    //!
    //! You basically want to initiate connection to other side using Connect method, then you
    //! can send data (Hashes) using SendPacket to other side and receive them using
    //! event Event_Incoming
    //!
    //! On server side, you need to create your own listener, that will create instance of GP
    //! class with QTcpSocket in constructor, then instead of "Connect" call "ResolveSignals"
    //! to connect event handler for socket operations, since then GP class will handle socket
    //! on its own
    //!
    //!
    //! See this sources for example implementation
    //! server: https://github.com/grumpy-irc/grumpy/blob/master/src/grumpyd/session.cpp
    //! client: https://github.com/grumpy-irc/grumpy/blob/master/src/libcore/grumpydsession.cpp
    class GPSHARED_EXPORT GP : public QObject
    {
            Q_OBJECT
        public:
            GP(QTcpSocket *tcp_socket = 0, bool mt = false);
            virtual ~GP();
            /*!
             * \brief Connect to remote server - only needed by clients
             * \param host remote server address or IP
             * \param port tcp port
             * \param ssl  whether SSL is enabled
             */
            virtual void Connect(QString host, int port, bool ssl);
            virtual bool IsConnected() const;
            virtual bool SendPacket(QHash<QString, QVariant> packet);
            virtual void SendProtocolCommand(gp_command_t command);
            virtual void SendProtocolCommand(gp_command_t command, QHash<QString, QVariant> parameters);
            //! Perform connection of Qt signals to internal functions,
            //! use this only if you aren't overriding this class
            virtual void ResolveSignals();
            virtual void Disconnect();
            virtual void SetCompression(int level);
            virtual void ResetCounters();
            unsigned long long GetBytesSent();
            unsigned long long GetBytesRcvd();
            unsigned long long GetCompBytesSent() const;
            unsigned long long GetCompBytesRcvd() const;
            unsigned long long GetPacketsSent() const;
            unsigned long long GetPacketsRecv() const;
            virtual bool IsReceiving();
            virtual quint32 GetIncomingPacketSize();
            virtual quint32 GetIncomingPacketRecv();
            virtual int GetVersion();
            quint32 MaxIncomingCacheSize;
            friend class libgp::Thread;

        signals:
            void Event_Connected();
            void Event_Disconnected();
            void Event_SocketError(QAbstractSocket::SocketError er);
            void Event_Timeout();
            void Event_ConnectionFailed(QString reason, int ec);
            void Event_Incoming(QHash<QString, QVariant> packet);
            void Event_SslHandshakeFailure(QList<QSslError> el, bool *is_ok);
            void Event_IncomingCommand(gp_command_t text, QHash<QString, QVariant> parameters);

        protected slots:
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
            QHash<QString, QVariant> packetFromRawBytes(QByteArray packet, int compression_level);
            void processHeader(QByteArray data);
            QByteArray mtPop();
            QMutex *mtLock;
            QList<QByteArray> mtBuffer;
            QMutex *mutex;
            quint32 incomingPacketSize;
            //! This is a minimum size required for data so that they get compressed, for performance reasons
            int minimumSizeForComp;
            gp_byte_t incomingPacketCompressionLevel;
            gp_byte_t compression;
            QByteArray incomingCache;
            QTcpSocket *socket;

        private:
#ifdef GP_WITH_STAT
            unsigned long long largePacketSize;
            unsigned long long largestPacketSize;
            QDateTime currentPacketTime;
#endif
            bool isSSL;
            QTimer *timer;
            unsigned int timeout;
            unsigned long long pingID;
            unsigned long long lastPTS;
            QDateTime lastPing;
            unsigned long long sentBytes;
            unsigned long long sentPackets;
            unsigned long long recvPackets;
            unsigned long long recvBytes;
            unsigned long long sentCmprBytes;
            unsigned long long recvCmprBytes;
            unsigned long long recvRAWBytes;
            Thread *thread;
            bool isMultithreaded;
    };
}

#endif // GP_H

