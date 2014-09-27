#-------------------------------------------------
#
# Project created by QtCreator 2014-09-27T02:10:11
#
#-------------------------------------------------


TARGET = myServer

TEMPLATE = app
CONFIG   += console
CONFIG   -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 \
                  -O3

LIBS += -levent



SOURCES += main.cpp \
    server.cpp \
    httplogic.cpp

HEADERS += \
    server.h \
    httplogic.h
