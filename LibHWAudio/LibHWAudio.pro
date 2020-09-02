#-------------------------------------------------
#
# Project created by QtCreator 2020-06-05T10:52:41
#
#-------------------------------------------------

CONFIG -= qt

TARGET = LibHWAudio
TEMPLATE = lib

DEFINES += LIBHWAUDIO_LIBRARY
DESTDIR = $$PWD/../Lib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        LibHWAudio.cpp \
    xpdma.c \
    LibHWAudio_global.cpp \
    Wave.cpp

HEADERS += \
        LibHWAudio.h \
    xpdma.h \
    CommDef.h \
    LibHWAudio_global.h \
    Wave.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
