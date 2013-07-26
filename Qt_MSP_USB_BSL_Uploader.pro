#-------------------------------------------------
#
# Project created by QtCreator 2013-07-25T20:47:29
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MSP_Firmware_Upload
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        qintelhexparser.cpp

HEADERS  += mainwindow.h \
        qintelhexparser.h

FORMS    += mainwindow.ui

LIBS += -lusb-1.0

RESOURCES += \
    resources.qrc
