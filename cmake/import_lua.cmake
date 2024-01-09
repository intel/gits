# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED LUA_ABS_PATH)
  install_dependencies("--with-lua")
  set(LUA_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua")
endif()

if(WIN32)
  add_definitions(-DLUA_USE_WINDOWS)
elseif(UNIX)
  add_definitions(-DLUA_USE_LINUX)
endif()

add_library(lua
  ${LUA_ABS_PATH}/ldo.c
  ${LUA_ABS_PATH}/lundump.c
  ${LUA_ABS_PATH}/lstring.c
  ${LUA_ABS_PATH}/lstate.c
  ${LUA_ABS_PATH}/lbaselib.c
  ${LUA_ABS_PATH}/lparser.c
  ${LUA_ABS_PATH}/lvm.c
  ${LUA_ABS_PATH}/ltm.c
  ${LUA_ABS_PATH}/lzio.c
  ${LUA_ABS_PATH}/ldump.c
  ${LUA_ABS_PATH}/ldebug.c
  ${LUA_ABS_PATH}/liolib.c
  ${LUA_ABS_PATH}/lfunc.c
  ${LUA_ABS_PATH}/lua.c
  ${LUA_ABS_PATH}/lctype.c
  ${LUA_ABS_PATH}/lobject.c
  ${LUA_ABS_PATH}/lcorolib.c
  ${LUA_ABS_PATH}/lmathlib.c
  ${LUA_ABS_PATH}/lauxlib.c
  ${LUA_ABS_PATH}/ltablib.c
  ${LUA_ABS_PATH}/lapi.c
  ${LUA_ABS_PATH}/loslib.c
  ${LUA_ABS_PATH}/ltests.c
  ${LUA_ABS_PATH}/lutf8lib.c
  ${LUA_ABS_PATH}/ltable.c
  ${LUA_ABS_PATH}/ldblib.c
  ${LUA_ABS_PATH}/lstrlib.c
  ${LUA_ABS_PATH}/lopcodes.c
  ${LUA_ABS_PATH}/llex.c
  ${LUA_ABS_PATH}/lmem.c
  ${LUA_ABS_PATH}/lcode.c
  ${LUA_ABS_PATH}/linit.c
  ${LUA_ABS_PATH}/loadlib.c
  ${LUA_ABS_PATH}/lgc.c
)
set_target_properties(lua PROPERTIES FOLDER External)

include_directories(${LUA_ABS_PATH})