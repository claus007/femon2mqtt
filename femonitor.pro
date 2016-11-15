QT += core
QT -= gui

CONFIG += c++11 debug

TARGET = femonitor
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

LIBS += -lmosquitto -ldvbapi
