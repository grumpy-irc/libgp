//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2015

#include "thread.h"
#include "gp.h"

using namespace libgp;

Thread::Thread(GP *gp)
{
    this->owner = gp;
}

Thread::~Thread()
{

}

void Thread::run()
{
    while (this->isRunning())
    {
        QByteArray incoming = this->owner->mtPop();
        if (incoming.isEmpty())
        {
            this->msleep(100);
            continue;
        }
        // These 2 calls are probably CPU intensive
        QHash<QString, QVariant> packet = this->owner->packetFromRawBytes(incoming);
        this->owner->processPacket(packet);
    }
}

