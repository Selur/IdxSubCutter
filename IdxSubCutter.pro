TEMPLATE = app
TARGET = IdxSubCutter
QT += core
CONFIG += console

CONFIG += precompile_header
win32-msvc* {
    message(Building for Windows using Qt $$QT_VERSION)
    CONFIG += c++11
    QMAKE_CFLAGS_RELEASE += -MT
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -MT;
    QMAKE_CXXFLAGS += /bigobj
    QMAKE_CFLAGS_DEBUG = -Zi \
        -MTd
    QMAKE_LFLAGS += /LARGEADDRESSAWARE # use more than 2GB of RAM
    QMAKE_LFLAGS += /DYNAMICBASE:NO
    contains(QMAKE_HOST.arch, x86_64):QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS,5.02 # Windows XP 64bit
    else:QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS,5.01 # Windows XP 32bit
    QMAKE_LFLAGS += /FIXED

   lessThan(QT_MAJOR_VERSION, 6) {
    CONFIG += c++11 # C++11 support
   } else {
    CONFIG += c++17 # C++11 support
    QMAKE_CXXFLAGS += /std:c++17
    QMAKE_CXXFLAGS += /Zc:__cplusplus
   }
}
greaterThan(QT_MAJOR_VERSION, 4) { # QT5+
    QT += widgets # for all widgets
    win32-msvc*:DEFINES += NOMINMAX
}
CODECFORSRC = UTF-8
CODECFORTR = UTF-8

HEADERS += Cutter.h
SOURCES += Cutter.cpp \
    main.cpp
FORMS +=
RESOURCES +=
