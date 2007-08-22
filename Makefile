CPP = gcc-2.95.3

#this is gcc 3.2
#CPP = gcc

ARCHFLAG = i586

METAMOD_SRCDIR = /usr/src/hl_bot_build/metamod-1.17/metamod

HLSDK_BASEDIR = /usr/src/hl_bot_build/hlsdk-2.3

BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp
#CPPFLAGS = ${BASEFLAGS} -march=i386 -O2 -w -I"../metamod" -I"../../devtools/hlsdk-2.3/singleplayer/common" -I"../../devtools/hlsdk-2.3/singleplayer/dlls" -I"../../devtools/hlsdk-2.3/singleplayer/engine" -I"../../devtools/hlsdk-2.3/singleplayer/pm_shared"
CPPFLAGS = ${BASEFLAGS} -march=${ARCHFLAG} -O2 -w -I"${METAMOD_SRCDIR}" -I"${HLSDK_BASEDIR}/multiplayer/common" -I"${HLSDK_BASEDIR}/multiplayer/dlls" -I"${HLSDK_BASEDIR}/multiplayer/engine" -I"${HLSDK_BASEDIR}/multiplayer/pm_shared"

OBJ = NodeMachine.o \
	bot.o \
	bot_buycode.o \
	bot_client.o \
	bot_func.o \
	bot_navigate.o \
	dll.o \
	engine.o \
	game.o \
	util.o \
	ChatEngine.o \
	IniParser.o

realbot_mm_i386.so: ${OBJ}
	${CPP} -fPIC -shared -o $@ ${OBJ} -Xlinker -Map -Xlinker realbot_mm.map -ldl
	mkdir -p Release
	mv $@ ${OBJ} realbot_mm.map Release

clean:
	rm -f *.o
	rm -f *.map
	rm -f *.so
	mv Release/*.so .
	rm -f Release/*
	mv *.so Release

distclean:
	rm -rf Release
	mkdir -p Release

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@
