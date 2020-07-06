#-------------------------------------------------
#
# Project created by QtCreator 2016-05-30T12:20:44
#
#-------------------------------------------------

QT       += core gui network
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = socks5_1
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    ProxyServer.cpp \
    ProxyConnection.cpp
#    BindProxyConnection.cpp \
#    TcpProxyConnection.cpp \
#    UdpProxyConnection.cpp

HEADERS  += widget.h \
    ProxyServer.h \
    ProxyConnection.h
#    BindProxyConnection.h \
#    TcpProxyConnection.h \
#    UdpProxyConnection.h

FORMS    += widget.ui
