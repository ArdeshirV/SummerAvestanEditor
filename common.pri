QT += core gui widgets
CONFIG += c++17
CONFIG -= app_bundle

AVESTAN_CORE = \
    $$PWD/src/config.cpp \
    $$PWD/src/document.cpp \
    $$PWD/src/editor.cpp \
    $$PWD/src/keyboard.cpp \
    $$PWD/src/theme.cpp

AVESTAN_HEADERS = \
    $$PWD/src/config.h \
    $$PWD/src/document.h \
    $$PWD/src/editor.h \
    $$PWD/src/keyboard.h \
    $$PWD/src/theme.h \
    $$PWD/src/unicode_util.h
