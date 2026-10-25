include(common.pri)

TEMPLATE = app
# The build system overrides this per platform (TARGET=... on the qmake
# command line); plain qmake builds produce `avestan-editor`.
TARGET = avestan-editor

INCLUDEPATH += $$PWD/src

SOURCES += \
    $$PWD/src/main.cpp \
    $$PWD/src/mainwindow.cpp \
    $$AVESTAN_CORE

HEADERS += \
    $$PWD/src/mainwindow.h \
    $$AVESTAN_HEADERS

RESOURCES += $$PWD/resources.qrc

win32 {
    RC_FILE = $$PWD/avestan-editor.rc
}
