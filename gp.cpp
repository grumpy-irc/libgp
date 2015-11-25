//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2015

#include <QTcpSocket>
#include <QSslSocket>
#include <QDataStream>
#include <QMutex>
#include <QTimer>
#include "thread.h"
#include "gp_exception.h"
#include "gp.h"

using namespace libgp;

// This code prevents the hacker compression bomb vulnerability of compression libs
#ifdef GP_PREVENT_HACK

#define GP_SHIELD(package) if (((unsigned long)(package[0] << 24) | (package[1] << 16) | (package[2] <<  8) | (package[3])) > this->MaxIncomingCacheSize) \
                             {\
                                this->closeError("Too big packet", GP_ERROR);\
                                return QHash<QString, QVariant>();\
                             }
#else
#define GP_SHIELD(package)
#endif

GP::GP(QTcpSocket *tcp_socket, bool mt)
{
    this->socket = tcp_socket;
    this->sentBytes = 0;
    this->recvBytes = 0;
    this->sentCmprBytes = 0;
    this->recvCmprBytes = 0;
    // We don't want to receive single packet bigger than 800kb
    this->MaxIncomingCacheSize = 800 * 1024;
    this->incomingPacketSize = 0;
    this->minimumSizeForComp = 64;
    this->recvRAWBytes = 0;
    this->timeout = 60;
    this->incomingPacketCompressionLevel = 0;
    this->mutex = new QMutex(QMutex::Recursive);
    this->mtLock = new QMutex(QMutex::Recursive);
    this->compression = 0;
    this->isSSL = false;
    this->timer = NULL;
    if (!mt)
    {
        this->thread = NULL;
    }
    else
    {
        this->thread = new Thread(this);
        this->thread->start();
    }
    this->isMultithreaded = mt;
}

GP::~GP()
{
    delete this->thread;
    delete this->timer;
    delete this->mtLock;
    delete this->socket;
    delete this->mutex;
}

void GP::Connect(QString host, int port, bool ssl)
{
    if (this->IsConnected())
        throw new libgp::GP_Exception("You can't connect using protocol that is already connected");
    this->isSSL = ssl;
    if (ssl)
        this->socket = new QSslSocket();
    else
        this->socket = new QTcpSocket();
    this->ResolveSignals();
    if (!ssl)
    {
        connect(this->socket, SIGNAL(connected()), this, SLOT(OnConnected()));
        this->socket->connectToHost(host, port);
    }
    else
    {
        connect(((QSslSocket*)this->socket), SIGNAL(encrypted()), this, SLOT(OnConnected()));
        connect(((QSslSocket*)this->socket), SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(OnSslHandshakeFailure(QList<QSslError>)));
        ((QSslSocket*)this->socket)->connectToHostEncrypted(host, port);
        //if (!((QSslSocket*)this->socket)->waitForEncrypted())
        //    this->closeError("SSL handshake failed: " + this->socket->errorString(), GP_ESSLHANDSHAKEFAILED);
    }
    this->lastPTS = 0;
    if (this->timeout > 0)
    {
        this->lastPing = QDateTime::currentDateTime();
        delete this->timer;
        this->pingID = 0;
        this->timer = new QTimer();
        connect(this->timer, SIGNAL(timeout()), this, SLOT(OnPingSend()));
        this->timer->start(13000);
    }
}

bool GP::IsConnected() const
{
    if (!this->socket)
        return false;
    return this->socket->isOpen();
}

void GP::OnPingSend()
{
    if (this->lastPing.secsTo(QDateTime::currentDateTime()) > this->timeout)
    {
        this->closeError("Ping timeout", GP_ERROR);
        return;
    }
    QHash<QString, QVariant> pack;
    pack.insert("type", QVariant(GP_TYPE_PING));
    pack.insert("n", QVariant(++this->pingID));
    pack.insert("p", QVariant(QDateTime::currentDateTime()));
    this->SendPacket(pack);
}

void GP::OnError(QAbstractSocket::SocketError er)
{
    emit this->Event_SocketError(er);
}

void GP::OnReceive()
{
    this->processIncoming(this->socket->readAll());
}

void GP::OnSslHandshakeFailure(QList<QSslError> el)
{
    bool ok = true;
    emit this->Event_SslHandshakeFailure(el, &ok);

    if (ok)
        ((QSslSocket*)this->socket)->ignoreSslErrors();
}

void GP::OnConnected()
{
    emit this->Event_Connected();
}

void GP::OnDisconnect()
{
    emit this->Event_Disconnected();
}

void GP::OnIncomingCommand(gp_command_t text, QHash<QString, QVariant> parameters)
{
    emit this->Event_IncomingCommand(text, parameters);
}

void GP::processPacket()
{
    if (this->isMultithreaded)
    {
        // Store this byte array into fifo for later processing by processor thread
        this->mtLock->lock();
        this->mtBuffer.append(this->incomingCache);
        this->mtLock->unlock();
        this->incomingPacketSize = 0;
        this->incomingCache.clear();
        return;
    }

    QHash<QString, QVariant> pack = this->packetFromIncomingCache();
    this->processPacket(pack);
}

void GP::processPacket(QHash<QString, QVariant> pack)
{
    if (!pack.contains("type"))
        throw new GP_Exception("Broken packet");

    emit this->Event_Incoming(pack);

    int type = pack["type"].toInt();
    this->lastPing = QDateTime::currentDateTime();
    switch (type)
    {
        case GP_TYPE_SYSTEM:
        {
            if (!pack.contains("cid"))
                this->closeError("Broken packet", GP_ERROR);
            QHash<QString, QVariant> parameters;
            if (pack.contains("parameters"))
                parameters = pack["parameters"].toHash();
            this->OnIncomingCommand(pack["cid"].toUInt(), parameters);
        }
            break;
        case GP_TYPE_PING:
        {
            if (pack.contains("p"))
            {
                QHash<QString, QVariant> re;
                re.insert("type", QVariant(GP_TYPE_PING));
                re.insert("n", pack["n"]);
                re.insert("o", pack["p"]);
                this->SendPacket(re);
            }
            else if (pack.contains("o"))
            {
                this->lastPTS = (unsigned long long)pack["o"].toDateTime().msecsTo(QDateTime::currentDateTime());
            }
            else
            {
                this->closeError("Broken ping reply", GP_ERROR);
            }
        }
            break;
    }
}

void GP::processIncoming(QByteArray data)
{
    this->recvRAWBytes += static_cast<unsigned long long>(data.size());
    if (this->incomingPacketSize)
    {
        // we are already receiving a packet
        int remaining_packet_data = this->incomingPacketSize - this->incomingCache.size();
        if (remaining_packet_data < 0)
            throw new GP_Exception("Negative packet size");
        if (data.size() == remaining_packet_data)
        {
            // this is extremely rare
            // the packet we received is exactly the remaining part of a block of data we are receiving
            this->incomingCache.append(data);
            this->processPacket();
        } else if (data.size() < remaining_packet_data)
        {
            // just append and skip
            this->incomingCache.append(data);
        } else if (data.size() > remaining_packet_data)
        {
            // this is most common and yet most annoying situation, the data we received are bigger than
            // packet, so we need to cut the remaining part and process
            QByteArray remaining_part = data.mid(0, remaining_packet_data);
            this->incomingCache.append(remaining_part);
            this->processPacket();
            data = data.mid(remaining_packet_data);
            this->processIncoming(data);
        }
    } else
    {
        int current_data_size = this->incomingCache.size() + data.size();
        int remaining_header_size = GP_HEADER_SIZE - this->incomingCache.size();
        // we just started receiving a new packet
        if (current_data_size < GP_HEADER_SIZE)
        {
            // great, the data is so small they don't even contain the header yet, let's cache this and wait for later
            this->incomingCache.append(data);
        } else if (current_data_size == GP_HEADER_SIZE)
        {
            // we received just a header
            this->incomingCache.append(data);
            this->processHeader(this->incomingCache);
        } else if (current_data_size > GP_HEADER_SIZE)
        {
            // we received a header and some block of data
            // first grab the remaining bytes that belong to header and cut them out
            QByteArray remaining_header_data = data.mid(0, remaining_header_size);
            data = data.mid(remaining_header_size);
            // now construct the header from cache and newly retrieved bytes
            QByteArray header = this->incomingCache + remaining_header_data;
            // now we have the header and remaining data, so continue processing
            this->processHeader(header);
            this->processIncoming(data);
        }
    }
}

void GP::closeError(QString error, int code)
{
    if (this->timer)
    {
        this->timer->stop();
        delete this->timer;
        this->timer = NULL;
    }
    if (!this->socket)
        return;
    if (this->socket->isOpen())
        this->socket->close();
    this->socket->deleteLater();
    this->socket = NULL;
    emit this->Event_ConnectionFailed(error, code);
}

static QByteArray ToArray(QHash<QString, QVariant> data)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::ReadWrite);
    GP_INIT_DS(stream);
    stream << data;
    return result;
}

static QByteArray ToArray(int number)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::ReadWrite);
    GP_INIT_DS(stream);
    stream << qint64(number);
    return result;
}

QHash<QString, QVariant> GP::packetFromIncomingCache()
{
    // Uncompress the data first
    if (this->incomingPacketCompressionLevel)
    {
        this->recvCmprBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(this->incomingCache.size());
        GP_SHIELD(this->incomingCache);
        this->incomingCache = qUncompress(this->incomingCache);
        this->recvBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(this->incomingCache.size());
    } else
    {
        this->recvBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(this->incomingCache.size());
    }
    QDataStream stream(&this->incomingCache, QIODevice::ReadWrite);
    GP_INIT_DS(stream);
    QHash<QString, QVariant> data;
    stream >> data;
    this->incomingPacketSize = 0;
    this->incomingPacketCompressionLevel = 0;
    this->incomingCache.clear();
    return data;
}

QHash<QString, QVariant> GP::packetFromRawBytes(QByteArray packet, int compression_level)
{
    if (compression_level)
    {
        GP_SHIELD(packet);
        this->recvCmprBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(packet.size());
        packet = qUncompress(packet);
        this->recvBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(packet.size());
    } else
    {
        this->recvBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(packet.size());
    }
    QDataStream stream(&packet, QIODevice::ReadWrite);
    GP_INIT_DS(stream);
    QHash<QString, QVariant> data;
    stream >> data;
    return data;
}

void GP::processHeader(QByteArray data)
{
    qint64 header, compression_level;
    QDataStream stream(&data, QIODevice::ReadWrite);
    GP_INIT_DS(stream);
    stream >> header >> compression_level;
    if (header < 0)
        this->closeError("Negative header size", GP_PROTOCOL_SERIOUS_FAILURE);
    if (compression_level < 0 || compression_level > 9)
        this->closeError("Invalid compression level", GP_PROTOCOL_SERIOUS_FAILURE);
    if (header > this->MaxIncomingCacheSize)
    {
        this->closeError("Too big packet", GP_ERROR);
    }
    this->incomingPacketCompressionLevel = compression_level;
    this->incomingCache.clear();
    this->incomingPacketSize = header;
}

QByteArray GP::mtPop()
{
    QByteArray result;
    this->mtLock->lock();
    if (this->mtBuffer.size() > 0)
    {
        result = this->mtBuffer.at(0);
        this->mtBuffer.removeAt(0);
    }
    this->mtLock->unlock();
    return result;
}

bool GP::SendPacket(QHash<QString, QVariant> packet)
{
    if (!this->socket)
        return false;
    // Current format of every packet is extremely simple
    // First GP_HEADER_SIZE bytes are the size of packet and compression level
    // Following bytes are the packet itself
    QByteArray result = ToArray(packet);
    bool using_compression = this->compression;
    if (result.size() < this->minimumSizeForComp)
        using_compression = false;
    if (using_compression)
    {
        this->sentBytes += GP_HEADER_SIZE + static_cast<unsigned long long>(result.size());
        result = qCompress(result, this->compression);
    }
    // Header contains 2 integers, first one is a size of whole packet (compressed if compression is used)
    // next one is an identifier of compression used
    QByteArray header;
    if (using_compression)
        header = ToArray(result.size()) + ToArray(this->compression);
    else
        header = ToArray(result.size()) + ToArray(0);
    if (header.size() != GP_HEADER_SIZE)
        throw new GP_Exception("Invalid header size: " + QString::number(header.size()));
    result.prepend(header);
    // We must lock the connection here to prevent multiple threads from writing into same socket thus writing borked data
    // into it
    this->mutex->lock();
    if (!using_compression)
        this->sentBytes += static_cast<unsigned long long>(result.size());
    else
        this->sentCmprBytes += static_cast<unsigned long long>(result.size());
    this->socket->write(result);
    this->socket->flush();
    this->mutex->unlock();
    return true;
}

void GP::SendProtocolCommand(unsigned int command)
{
    this->SendProtocolCommand(command, QHash<QString, QVariant>());
}

void GP::SendProtocolCommand(unsigned int command, QHash<QString, QVariant> parameters)
{
    QHash<QString, QVariant> pack;
    pack.insert("type", QVariant(GP_TYPE_SYSTEM));
    pack.insert("cid", QVariant(command));
    pack.insert("parameters", QVariant(parameters));
    this->SendPacket(pack);
}

void GP::ResolveSignals()
{
    if (!this->socket)
        throw new GP_Exception("this->socket");
    connect(this->socket, SIGNAL(readyRead()), this, SLOT(OnReceive()));
    connect(this->socket, SIGNAL(disconnected()), this, SLOT(OnDisconnect()));
    connect(this->socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(OnError(QAbstractSocket::SocketError)));
}

void GP::Disconnect()
{
    if (!this->socket)
        return;
    if (this->socket->isOpen())
        this->socket->close();
    this->socket->deleteLater();
    this->socket = NULL;
}

void GP::SetCompression(int level)
{
    this->compression = level;
}

unsigned long long GP::GetBytesSent()
{
    return this->sentBytes;
}

unsigned long long GP::GetBytesRcvd()
{
    return this->recvBytes;
}

unsigned long long GP::GetCompBytesSent() const
{
    return this->sentCmprBytes;
}

unsigned long long GP::GetCompBytesRcvd() const
{
    return this->recvCmprBytes;
}

int GP::GetVersion()
{
    return GP_VERSION;
}


