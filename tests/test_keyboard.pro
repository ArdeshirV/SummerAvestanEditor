QT += testlib
include(../common.pri)

TEMPLATE = app
TARGET = test_keyboard
INCLUDEPATH += $$PWD/../src

SOURCES += test_keyboard.cpp $$AVESTAN_CORE
HEADERS += $$AVESTAN_HEADERS
RESOURCES += $$PWD/../resources.qrc

CONFIG += console
CONFIG -= app_bundle
