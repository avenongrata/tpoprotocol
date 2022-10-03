
SDK_DIR  = /opt/radiomodule-sdk
SDK_BIN  = $(SDK_DIR)/bin
SDK_PREF = $(SDK_BIN)/arm-linux-

#---------------------------------------------------------------------

TARGET   = liblogger.so
DISTDIR  = ./app_dir/lib/

INCPATH  = -I. -I../ -I./p7
LIBS     = -L./app_dir/lib/

OBJECTS  = logger.o

#---------------------------------------------------------------------

DEFINES  =
CFLAGS   = -pipe -g -Wall -Wextra -fPIC $(DEFINES)
CXXFLAGS = -std=gnu++1z -fPIC $(CFLAGS)
LFLAGS   = -shared -Wl,-soname,$(TARGET)

SDK_GCC  = $(SDK_PREF)gcc -c $(CFLAGS) $(INCPATH)
SDK_GXX  = $(SDK_PREF)g++ -c $(CXXFLAGS) $(INCPATH)
SDK_LINK = $(SDK_PREF)g++ $(LFLAGS) $(INCPATH)

TAR_FILE = $(DISTDIR)/$(TARGET)

#==============================================================================

SUBDIRS =	api-library	\
		app-library	\
		device-library	\
		global-module	\
		memory-library	\
		netsock-library	\
		p7

CO_LIBS = 	api 	\
		app	\
		dev	\
		global	\
		memory	\
		netsock \
		pseven

#==============================================================================

first:	distclean pseven $(OBJECTS)
	test -d $(DISTDIR) || mkdir -p $(DISTDIR);
	$(SDK_LINK) -o $(TAR_FILE) $(OBJECTS) $(LIBS)

pseven_clean:
	THIS_PATH="$$(pwd)"; cd p7 && make distclean; cd $${THIS_PATH};
	
clean:
	rm -f $(OBJECTS)

distclean: pseven_clean clean
	rm -f $(TAR_FILE)
	
#==============================================================================
	
full:	full_distclean distclean $(CO_LIBS) $(OBJECTS)
	
project:
	THIS_PATH="$$(pwd)"; cd $(DIR) && make $(ARGS); cd $${THIS_PATH};
	
#==============================================================================

full_clean:
	for DIR in $(SUBDIRS); \
	do \
	    DIR="$${DIR}" ARGS="clean" $(MAKE) project; \
	done;

full_distclean: full_clean
	for DIR in $(SUBDIRS); \
	do \
	    DIR="$${DIR}" ARGS="distclean" $(MAKE) project; \
	done;
	
#==============================================================================
# Libraries

api:
	DIR="api-library" $(MAKE) project;

app:
	DIR="app-library" $(MAKE) project;

dev:
	DIR="device-library" $(MAKE) project;

global:
	DIR="global-module" $(MAKE) project;

memory:
	DIR="memory-library" $(MAKE) project;

netsock:
	DIR="netsock-library" $(MAKE) project;

pseven:
	DIR="p7" $(MAKE) project;

#==============================================================================
# Logger as library

logger.o:
	$(SDK_GXX) -o logger.o logger.cpp
