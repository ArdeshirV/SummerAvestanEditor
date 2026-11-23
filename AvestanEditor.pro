include(common.pri)

TEMPLATE = app
# The build system overrides this per platform (TARGET=... on the qmake
# command line); plain qmake builds produce `avestan-editor`.
TARGET = avestan-editor

INCLUDEPATH += $$PWD/src

SOURCES += \
    $$PWD/src/main.cpp \
    $$PWD/src/mainwindow.cpp \
    $$PWD/src/dialogs.cpp \
    $$AVESTAN_CORE

HEADERS += \
    $$PWD/src/mainwindow.h \
    $$PWD/src/dialogs.h \
    $$AVESTAN_HEADERS

RESOURCES += $$PWD/resources.qrc

android {
    QT += core-private
    SOURCES += $$PWD/JNI/safbridge.cpp
    HEADERS += $$PWD/JNI/safbridge.h
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

win32 {
    RC_FILE = $$PWD/avestan-editor.rc
}
