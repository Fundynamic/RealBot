# vi: set ts=4 sw=4 :
# vim: set tw=75 :

# MetaMod makefile
# Copyright (c) 2001-2003 Will Day <willday@hpgx.net>
#
# based on the Valve SDK 2.1 Makefile as well as the Makefile
# in adminmod by Alfred Reynolds.
#
# From SDK 2.1 dlls/Makefile:
#! Half-Life StandardSDK 2.0 mp_i386.so Makefile for i386 Linux
#! April 2000 by Leon Hartwig (jehannum@planethalflife.com)


# NOTE: This is a generic Makefile for metamod and the bundled plugins, and
# is symlinked into each subdir.  Per-module config statements are in
# Config.mak in each subdir.

ifdef COMSPEC
	OS=windows
else
	OS=linux
endif

#############################################################################
# CONFIGURATION
#############################################################################

# set paths for your environment
ifeq "$(OS)" "linux"
	PATH_WIN=/usr/local/cross-tools/i586-mingw32msvc/bin
	SDKTOP=../../hlsdk
	INST_DIR=/usr/local/hlds_l/addons/metamod/dlls
else	## windows
	PATH_WIN=/gcc-2.95.2-msvcrt/bin
	SDKTOP=../../hlsdk
	INST_DIR=/hlserver/tfc/dlls
endif

DLLS_DIR=../dlls
METADIR=../metamod

# set compiler to use
CC=gcc


# set timezone string
ifeq "$(OS)" "linux"
	COMPILE_TZ=$(shell date +%z)
else
	COMPILE_TZ=
endif


## Developer overrrides
ifeq "$(USER)" "willday"
	CC=gcc-2.95
    ifeq "$(OS)" "linux"
       SDKTOP=$(HOME)/src/hl/hlsdk-2.3
       INST_DIR=$(HOME)/half-life/cstrike/dlls
       TEST_DIR=$(HOME)/test/tfc/dlls
       TST_DIR=$(HOME)/tmp
    else
       SDKTOP=/hlsdk
       TEST_DIR=/hlserver/tfc/dlls
    endif
endif
ifeq "$(USER)" "flori"
    PATH_WIN=/usr/local/cross-tools/i386-mingw32msvc/bin
    TEST_DIR=/data/hlds_l/Admin/dlls
endif

SDKSRC=$(SDKTOP)/multiplayer

#############################################################################
# OS DEPENDENCIES
#############################################################################

ARCH=i586
ARCHD=
ifeq "$(OS)" "linux"
	INSTALL=install -m 644
	LD_WINDLL= PATH=$(PATH_WIN) $(PATH_WIN)/dllwrap
	TARGET=linux
	LIBFILE=$(LIBFILE_LINUX)
	TARGET_FILE=$(TARGET_LINUX)
	ARCHD = $(shell uname -m)
	ifeq "$(ARCHD)" "x86_64"
		ARCH=x86_64
		ARCHD=.amd64
		MODARCH=amd64
	else
		ARCHD=
		MODARCH=i386
	endif
else	## windows
	INSTALL=cp
	LD_WINDLL= $(PATH_WIN)/dllwrap
	TARGET=win32
	LIBFILE=$(LIBFILE_WIN)
	TARGET_FILE=$(TARGET_WIN)
endif

CC_WIN=$(PATH_WIN)/gcc
RES_WIN=$(PATH_WIN)/windres
OBJDIR_LINUX_OPT=opt.linux$(ARCHD)
OBJDIR_WIN_OPT=opt.win32
OBJDIR_LINUX_DBG=debug.linux$(ARCHD)
OBJDIR_WIN_DBG=debug.win32


#############################################################################
# COMPILE OPTIONS
#############################################################################

GCCMAJ = $(shell gcc -dumpversion | sed -e 's/\.[0-9][0-9]*\.[0-9]//')

# original safe optimization, from valve Makefile
#CCOPT = -O2 -ffast-math -funroll-loops \
#	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2 \
#	-malign-jumps=2 -malign-functions=2

# safe optimization, adapted from adminmod Makefile
#CCOPT = -m486 -O6 -ffast-math -funroll-loops \
# -fexpensive-optimizations -malign-loops=2  -malign-jumps=2 \
# -malign-functions=2 -Wall

# full optimization, adapted from adminmod Makefile
# "WONT WORK WITH omit-frame-pointer"?
# - disable (unneeded) C++ exceptions and rtti code to save some space ?
ifeq "$(ARCH)" "x86_64"
	# K8 targets only available from gcc 3.4 on.
	CCOPT =
else
	CCOPT = -march=$(ARCH)
endif

CCOPT += $(CCO) -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations \
	-fno-exceptions -fno-rtti -s

ifeq "$(GCCMAJ)" "3"
	CCOPT += -falign-loops=2 -falign-jumps=2 -falign-functions=2 
else
	CCOPT += -malign-loops=2 -malign-jumps=2 -malign-functions=2 
endif

# optimization level; overridden for certain problematic files
CCO = -O6

# debugging; halt on warnings
CCDEBUG+= -ggdb3


#############################################################################
# COMPILE SETUP
#############################################################################


SRCDIR=.
INCLUDEDIRS=-I$(SRCDIR) -I$(METADIR) -I$(SDKSRC)/engine -I$(SDKSRC)/common -I$(SDKSRC)/pm_shared -I$(SDKSRC)/dlls -I$(SDKSRC)
FILES_ALL = *.cpp *.h [A-Z]* *.rc

CFLAGS=-Wall -Wno-unknown-pragmas 
#CFLAGS += -DTEST

ifeq "$(USER)" "flori"
	# extra warnings
	CFLAGS+=-W -Wundef \
		-Wshadow -Wpointer-arith -Wcast-qual -Wcast-align \
		-Wconversion -Wsign-compare \
		-Wstrict-prototypes -Wmissing-prototypes \
		-Wmissing-noreturn -Winline \
		-Wlong-long 
	ifeq "$(GCCMAJ)" "2"
		CFLAGS += -Wbad-function-cast -Wmissing-declarations -Wnested-externs
	else
		CFLAGS += -Wno-deprecated
	endif
	# extra C++ warnings
	CFLAGS+=-Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder \
		-Woverloaded-virtual -Wsign-promo -Wsynth -Weffc++
	# undesired warnings
	#CFLAGS+=-pedantic -Waggregate-return -Wredundant-decls -Wtraditional 
	# to help debugging; requires edited SDK
	CCDEBUG+=-Werror
	# link against older libc for compatibility
	#EXTRA_LINK = -L../../priv/libc -lc
endif

ifeq "$(USER)" "willday"
	# extra warnings
	CFLAGS+=-W -Wundef \
		-Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align \
		-Wconversion -Wsign-compare \
		-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
		-Wmissing-noreturn -Wnested-externs -Winline \
		-Wlong-long 
	# extra C++ warnings
	CFLAGS+=-Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder \
		-Woverloaded-virtual -Wsign-promo -Wsynth -Weffc++
	# undesired warnings
	#CFLAGS+=-pedantic -Waggregate-return -Wredundant-decls -Wtraditional 
	# to help debugging; requires edited SDK
	CCDEBUG+=-Werror
	# link against older libc for compatibility
	EXTRA_LINK = -L../priv/libc -lc
endif

ifeq "$(OPT)" "opt"
	ODEF = -DOPT_TYPE=\"optimized\"
	CFLAGS := $(CCOPT) $(CFLAGS) $(ODEF)
	OBJDIR_LINUX = $(OBJDIR_LINUX_OPT)
	OBJDIR_WIN = $(OBJDIR_WIN_OPT)
else	# debug
	ODEF = -DOPT_TYPE=\"debugging\"
	CFLAGS := $(CCDEBUG) $(CFLAGS) $(ODEF)
	OBJDIR_LINUX = $(OBJDIR_LINUX_DBG)
	OBJDIR_WIN = $(OBJDIR_WIN_DBG)
	DLLS_DIR := $(DLLS_DIR)/debug
endif

include Config.mak
# any local CFLAGS from Config.mak
CFLAGS += $(EXTRA_CFLAGS)

# provide timezone info
$(OBJDIR_LINUX)/vdate.o $(OBJDIR_WIN)/vdate.o: CFLAGS += -DCOMPILE_TZ=\"$(COMPILE_TZ)\"


# ignore complaints from SDK files like cbase.h
$(OBJDIR_LINUX)/sdk_util.o $(OBJDIR_WIN)/sdk_util.o: CFLAGS += -Wno-unused

# ignore complaints from STL headers 
STLOBJ = $(STLFILES:%.cpp=$(OBJDIR_LINUX)/%.o) 
STLOBJ += $(STLFILES:%.cpp=$(OBJDIR_WIN)/%.o) 
#$(OBJDIR_LINUX)/mreg.o $(OBJDIR_WIN)/mreg.o: CFLAGS += -Wno-effc++
$(STLOBJ): FILTER= 2>&1 | ../tools/stlfilter
$(STLOBJ): CFLAGS += -Wno-error

# these files seem to create "Internal compiler error" errors under mingw
# when using -O6
#$(OBJDIR_WIN)/engine_api.o: CCO = -O5
#$(OBJDIR_WIN)/dllapi_api.o: CCO = -O5

#############################################################################
# BUILDING LINUX SO
#############################################################################

# linux .so compile commands
DO_CC_LINUX=$(CC) $(CFLAGS) -fPIC $(INCLUDEDIRS) -o $@ -c $< $(FILTER)
LINK_LINUX=$(CC) $(CFLAGS) -shared -ldl -lm $(EXTRA_LINK) $(OBJ_LINUX) -o $@

# sort by date
ifeq "$(OS)" "linux"
SRCFILES := $(shell ls -t $(SRCFILES))
endif
# linux object files
OBJ_LINUX := $(SRCFILES:%.cpp=$(OBJDIR_LINUX)/%.o)

# compiling linux object files
$(OBJDIR_LINUX)/%.o: $(SRCDIR)/%.cpp
	$(DO_CC_LINUX)

# linux .so target file
LIBFILE_LINUX = $(MODNAME)_$(MODARCH).so
TARGET_LINUX = $(OBJDIR_LINUX)/$(LIBFILE_LINUX)


#############################################################################
# BUILDING WINDOWS DLL
#############################################################################

# windows .dll compile commands
DO_CC_WIN=$(CC_WIN) $(CFLAGS) $(INCLUDEDIRS) -o $@ -c $<
DO_RES_WIN=$(RES_WIN) '$(ODEF)' --include-dir . --include-dir ../metamod -i $< -O coff -o $@
#LINK_WIN=$(LD_WINDLL) -k -mwindows --add-stdcall-alias --def metamod.def -o $@ $(OBJ_WIN)
#LINK_WIN=$(LD_WINDLL) -A -k -mwindows --export-all-symbols -o $@ $(OBJ_WIN)
LINK_WIN=$(LD_WINDLL) -mwindows --add-stdcall-alias $(OBJ_WIN) $(RES_OBJ_WIN) -o $@

# windows object files
OBJ_WIN := $(SRCFILES:%.cpp=$(OBJDIR_WIN)/%.o)
RES_OBJ_WIN := $(RESFILE:%.rc=$(OBJDIR_WIN)/%.o)

# compiling windows object files
$(OBJDIR_WIN)/%.o: $(SRCDIR)/%.cpp
	$(DO_CC_WIN)

# compiling windows resource file
$(OBJDIR_WIN)/%.o: $(SRCDIR)/%.rc $(INFOFILES)
	$(DO_RES_WIN)

# windows .dll target file
LIBFILE_WIN = $(MODNAME).dll
TARGET_WIN = $(OBJDIR_WIN)/$(LIBFILE_WIN)


#############################################################################
# OVERRIDES
#############################################################################

ifeq "$(PLATFORM)" "linux-only"
	LIBFILE_WIN =
	TARGET_WIN =
endif
ifeq "$(PLATFORM)" "win32-only"
	LIBFILE_LINUX =
	TARGET_LINUX =
	TARGET=win32
	LIBFILE=$(LIBFILE_WIN)
	TARGET_FILE=$(TARGET_WIN)
endif


#############################################################################
# BUILD RULES
#############################################################################

default: $(TARGET_FILE)

ifeq "$(USER)" "willday"
include ../priv/metamod.mak
endif
ifeq "$(USER)" "flori"
include ../../priv/metamod.mak
endif

all: do_dll_linux do_dll_win32

opt:
	$(MAKE) default OPT=opt

linux: do_dll_linux
win32: do_dll_win32

linux_opt: 
	$(MAKE) linux OPT=opt
win32_opt: 
	$(MAKE) win32 OPT=opt

$(TARGET_LINUX): msgs/debug msgs/log msgs/error $(OBJDIR_LINUX) $(OBJ_LINUX)
	$(LINK_LINUX)

# for plugins, recompile meta_api.cpp if info_name.h changed
$(OBJDIR_LINUX)/meta_api.o $(OBJDIR_WIN)/meta_api.o: info_name.h

ifeq "$(USER)" "willday"
$(TARGET_LINUX): .tags .htags
endif

$(TARGET_WIN): $(OBJDIR_WIN) $(OBJ_WIN) $(RES_OBJ_WIN)
	$(LINK_WIN)

$(OBJDIR_LINUX) $(OBJDIR_WIN) msgs:
	mkdir -p $@

# make sure to recompile vdate.c for each link
$(OBJDIR_LINUX)/vdate.o $(OBJDIR_WIN)/vdate.o : $(SRCFILES) *.h 

ifdef LINKED_SRCFILES
# copy linked files
$(LINKED_SRCFILES): $(LINK_DEST_DIR)/$@
	cp -f $(LINK_DEST_DIR)/$@ $@
endif

domsgs: msgs/debug msgs/log msgs/error

msgs/debug: $(SRCFILES) *.h msgs
	egrep "DEBUG\([0-9]" $(SRCFILES) *.h | sed "s/:[ 	]*/	/" | sort -k2,2 > $@

msgs/log: $(SRCFILES) *.h msgs
	egrep "META_LOG\(" $(SRCFILES) *.h | sed "s/:[ 	]*/	/" | sort > $@

msgs/error: $(SRCFILES) *.h msgs
	egrep "META_ERROR\(" $(SRCFILES) *.h | sed "s/:[ 	]*/	/" | sort > $@

tags:	.tags .htags
ctags:	.tags
htags:	.htags

.tags: $(SRCFILES) *.h
	-ctags -a $?

.htags: *.h
	-htags $?

retags:
	-rm -f .tags .htags
	ctags -f .tags `find $(SDKSRC) -name '*.h'`
	ctags -f .tags -a `find $(METADIR) -name old -prune -o -name '*.h' -print`
	ctags -f .tags -a $(SRCFILES)
	htags -R $(SDKSRC)
	htags `find $(METADIR) -name old -prune -o -name '*.h' -print`
	htags *.h

clean: clean_$(TARGET)

clean_linux:
	test -n "$(OBJDIR_LINUX)"
#	-rm -f $(OBJDIR_LINUX)/*
	-rm -rf $(OBJDIR_LINUX)

clean_win32:
	test -n "$(OBJDIR_WIN)"
#	-rm -f $(OBJDIR_WIN)/*
	-rm -rf $(OBJDIR_WIN)

cleanall_linux:
	$(MAKE) clean_linux
	$(MAKE) clean_linux OPT=opt

cleanall_win32:
	$(MAKE) clean_win32
	$(MAKE) clean_win32 OPT=opt

cleanall: cleanall_linux cleanall_win32

dll_linux dll_win32: 
	$(MAKE) do_$@
	$(MAKE) do_$@ OPT=opt

do_dll_linux: $(DLLS_DIR) $(DLLS_DIR)/$(LIBFILE_LINUX)
do_dll_win32: $(DLLS_DIR) $(DLLS_DIR)/$(LIBFILE_WIN)

$(DLLS_DIR):
	mkdir -p $(DLLS_DIR)

$(DLLS_DIR)/$(LIBFILE_LINUX): $(TARGET_LINUX)
	$(INSTALL) $+ $@

$(DLLS_DIR)/$(LIBFILE_WIN): $(TARGET_WIN)
	$(INSTALL) $+ $@

dlls: dll_linux dll_win32
rmdlls:
	-rm -f ../dlls/*.* ../dlls/debug/*.*

spotless: cleanall
	-rmdir $(OBJDIR_LINUX) $(OBJDIR_WIN)
	-rm -f .snap $(GENERATED)

distclean:	spotless

install: $(INST_DIR)/$(LIBFILE)
test: $(TEST_DIR)/$(LIBFILE)
test_opt:
	$(MAKE) test OPT=opt

$(INST_DIR)/$(LIBFILE) $(TEST_DIR)/$(LIBFILE): $(TARGET_FILE)
	$(INSTALL) $< $@

tst: $(TST_DIR)/$(LIBFILE_WIN)

$(TST_DIR)/$(LIBFILE_WIN): $(TARGET_WIN)
	cp $< $@

snap:	.snap

.snap: $(FILES_ALL)
	mkdir -p snapshots
	tar zcvf snapshots/`date '+%m%d-%H%M'`.tgz $(FILES_ALL)
	touch .snap
	
depend: Rules.depend

ifeq "$(OS)" "linux"
Rules.depend: $(SRCFILES)
	$(CC) -MM $(INCLUDEDIRS) $(SRCFILES) | sed "s;\(^[^ 	]*\):\(.*\);$(OBJDIR_LINUX)/\1 $(OBJDIR_WIN)/\1: \2;" > $@
endif

include Rules.depend
