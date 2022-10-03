#======================================================================
# Makefile для ручной сборки.
#======================================================================

TARGET     = tpoprotocol
DESTDIR    = .

SDK_PATH   = /opt/radiomodule-sdk
SDK_BINS   = $(SDK_PATH)/bin
SDK_LIBS   = $(SDK_PATH)/arm-buildroot-linux-musleabihf/lib
SDK_INCS   = $(SDK_PATH)/arm-buildroot-linux-musleabihf/include
SDK_PREF   = $(SDK_BINS)/arm-linux-

SHELL      = /bin/bash
DEFINES    =
CFLAGS     = -O2 -Wall -Wextra $(DEFINES)
CXXFLAGS   = -std=gnu++1z -std=gnu++17 $(CFLAGS)
INCPATH    = -I. -I$(SDK_INCS) -I./p7 -I../control-program/ -I./logger-library/p7
LIBS       = -L. -lnetsock -lglobal -lapi -lapp -ldevice -lmemory -lpthread -lp7 -llogger
LFLAGS     = -O1 $(CXXFLAGS)

SDK_GCC    = $(SDK_PREF)gcc    $(CFLAGS)   $(LIBS) $(INCPATH)
SDK_GXX    = $(SDK_PREF)g++ -c $(CXXFLAGS) $(LIBS) $(INCPATH)
SDK_LINK   = $(SDK_PREF)g++    $(LFLAGS)   $(LIBS) $(INCPATH)

#======================================================================

OBJECTS = device.o udpserver.o protocol.o statistic.o timers.o core.o \
	  common.o devtree.o devapi.o

#======================================================================

first: all

all: distclean \
     application

application: $(OBJECTS)
	$(SDK_LINK) -o $(DESTDIR)/$(TARGET) $(OBJECTS) main.cpp

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f $(DESTDIR)/$(TARGET)

#======================================================================

udpserver.o:
	$(SDK_GXX) udpserver.cpp
	
protocol.o: udpserver.o statistic.o devtree.o
	$(SDK_GXX) protocol.cpp

device.o:
	$(SDK_GXX) device.cpp
	
statistic.o: device.o timers.o
	$(SDK_GXX) statistic.cpp
	
timers.o: timers.o
	$(SDK_GXX) timers.cpp
	
core.o: statistic.o protocol.o
	$(SDK_GXX) core.cpp
	
common.o: timers.o device.o
	$(SDK_GXX) common.cpp
	
devtree.o: devtree.cpp
	$(SDK_GXX) devtree.cpp
	
devapi.o:
	$(SDK_GXX) devapi.cpp

#======================================================================
