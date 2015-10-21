//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2015

#ifndef GP_EXCEPTION_H
#define GP_EXCEPTION_H

#include <QList>
#include <QString>

namespace libgp
{
    class GP_Exception
    {
        public:
            GP_Exception();
            GP_Exception(QString Message);

            virtual ~GP_Exception() {}
            virtual int ErrorCode() { return errc; }
            virtual QString GetMessage();
        private:
            int errc;
            QString message;
            QString stack_tree;
    };
}

#endif // GP_EXCEPTION_H
