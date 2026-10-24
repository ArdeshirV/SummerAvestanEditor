QT += testlib
include(../common.pri)

TEMPLATE = app
TARGET = test_editor
INCLUDEPATH += $$PWD/../src

SOURCES += test_editor.cpp $$AVESTAN_CORE
HEADERS += $$AVESTAN_HEADERS
RESOURCES += $$PWD/../resources.qrc

CONFIG += console
CONFIG -= app_bundle
