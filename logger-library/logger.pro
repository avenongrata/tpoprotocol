TEMPLATE = lib

TARGET = logger

CONFIG += console c++11 c++14 c++17
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$_PRO_FILE_PWD_/ \
               $$_PRO_FILE_PWD_/../ \
               $$_PRO_FILE_PWD_/p7/ \
               
LIBS += -L$$_PRO_FILE_PWD_/ \
        -L$$_PRO_FILE_PWD_/../app_dir/lib/

DEPENDPATH += $$PWD/

SOURCES += \
    logger.cpp

HEADERS += \
    logger.h

DESTDIR = $$_PRO_FILE_PWD_/../app_dir/lib/
target.path = $$DESTDIR
!isEmpty(target.path): INSTALLS += target
