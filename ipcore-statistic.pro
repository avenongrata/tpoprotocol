TEMPLATE = subdirs
# lib

TARGET = ipcore_statistic

CONFIG += console c++11 c++14 c++17
#CONFIG -= app_bundle
#CONFIG -= qt

SUBDIRS = global app api config netsock memory device logger

global.file                 = global-module/global.pro
api.file                    = api-library/api.pro
app.file                    = app-library/app.pro
device.file                 = device-library/device.pro
memory.file                 = memory-library/memory.pro
netsock.file                = netsock-library/netsock.pro
logger.file                 = logger-library/logger.pro
config.file                 = config-library/config.pro

SOURCES += \
    common.cpp \
    core.cpp \
    devapi.cpp \
    device.cpp \
    devtree.cpp \
    main.cpp \
    protocol.cpp \
    statistic.cpp \
    timers.cpp \
    udpserver.cpp

HEADERS += \
    common.h \
    core.h \
    devapi.h \
    device.h \
    devtree.h \
    protocol.h \
    statistic.h \
    timers.h \
    udpserver.h


# LIBS += -L$$_PRO_FILE_PWD_/../libs -ldevice

INCLUDEPATH += $$_PRO_FILE_PWD_/../ ./logger-library/p7

# DESTDIR = $$_PRO_FILE_PWD_/../libs

# target.path = /usr/lib
# !isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Makefile \
    build.ninja \
    app_dir/conf/logger.ini \
    app_dir/conf/tpoprotocol.ini
