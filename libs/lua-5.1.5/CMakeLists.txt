cmake_minimum_required(VERSION 3.5)

project(lua51)

set(LUA_SRC
	src/lapi.c
	src/lauxlib.c
	src/lbaselib.c
	src/lcode.c
	src/ldblib.c
	src/ldebug.c
	src/ldo.c
	src/ldump.c
	src/lfunc.c
	src/lgc.c
	src/linit.c
	src/liolib.c
	src/llex.c
	src/lmathlib.c
	src/lmem.c
	src/loadlib.c
	src/lobject.c
	src/lopcodes.c
	src/loslib.c
	src/lparser.c
	src/lstate.c
	src/lstring.c
	src/lstrlib.c
	src/ltable.c
	src/ltablib.c
	src/ltm.c
	src/lundump.c
	src/lvm.c
	src/lzio.c
	src/print.c
)

add_library(lua51 STATIC ${LUA_SRC})

if (UNIX)
	add_definitions(-DLUA_USE_MKSTEMP)
endif()

target_include_directories(lua51 PUBLIC src)
