# Created by and for Qt Creator. This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = stksamp

CONFIG-= QT
QT-= gui core

HEADERS = \
   $$PWD/stretch.h \
   $$PWD/sorter.h

SOURCES = \
   $$PWD/main.cpp \
   $$PWD/stretch.cpp \
   $$PWD/sorter.cpp

INCLUDEPATH = \
    $$PWD/.
    $$PWD/local

QMAKE_LIBDIR = \
    $$PWD/.
    /usr/local/lib

QMAKE_CXXFLAGS += -fpermissive -c
QMAKE_LFLAGS += -lstk -lcantamus -lfftw3 -L/usr/lib -lworld
#DEFINES = 


unix|win32: LIBS += -lstk

unix|win32: LIBS += -lcantamus

unix|win32: LIBS += -lfftw3

unix{
    target.path += /usr/bin
    INSTALLS +=  target
}
