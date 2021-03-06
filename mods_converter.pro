QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG += static
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
INCLUDEPATH += $$PWD/src
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    src/ConfigReader.cpp \
    src/FileIO.cpp \
    src/Generator.cpp

HEADERS += \
    src/ConfigReader.h \
    src/FileIO.h \
    src/Generator.h \
    src/types.h
