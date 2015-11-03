#-------------------------------------------------
#
# Project created by QtCreator 2015-10-21T15:52:25
#
#-------------------------------------------------

QT       += network

QT       -= gui

TARGET = gp
TEMPLATE = lib

DEFINES += GP_LIBRARY

SOURCES += gp.cpp \
    gp_exception.cpp \
    thread.cpp

HEADERS += gp.h\
        gp_global.h \
    gp_exception.h \
    thread.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
