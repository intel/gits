# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

list(APPEND common_SOURCES
  include/apis_iface.h
  include/argument.h
  include/bit_range.h
  include/buffer.h
  include/config.h
  include/configUtils.h
  include/diagnostic.h
  include/dynamic_linker.h
  include/exception.h
  include/function.h
  include/getopt_.h
  include/gits.h
  include/hashing.h
  include/id.h
  include/InputListener.h
  include/istdhash.h
  include/keyEvents.h
  include/key_value.h
  include/library.h
  include/log.h
  include/lua_bindings.h
  include/macros.h
  include/malloc_allocator.h
  include/mapped_file.h
  include/MemorySniffer.h
  include/message_pump.h
  include/performance.h
  include/platform.h
  include/pragmas.h
  include/resource_manager.h
  include/runner.h
  include/scheduler.h
  include/streams.h
  include/texture_converter.h
  include/timer.h
  include/token.h
  include/tools_lite.h
  include/tools.h
  include/version.h
  include/zone_allocator.h
  include/automateCCode.h
  include/ccodeWriteWrap.h

  apis_iface.cpp
  argument.cpp
  bit_range.cpp
  buffer.cpp
  config.cpp
  diagnostic.cpp
  exception.cpp
  function.cpp
  gits.cpp
  getopt.cpp
  hashing.cpp
  id.cpp
  library.cpp
  log.cpp
  mapped_file.cpp
  MemorySniffer.cpp
  message_pump.cpp
  performance.cpp
  resource_manager.cpp
  runner.cpp
  scheduler.cpp
  streams.cpp
  timer.cpp
  token.cpp
  tools_lite.cpp
  tools.cpp
  version.cpp
  dynamic_linker.cpp
  zone_allocator.cpp
  lua_bindings.cpp
  InputListener.cpp
  keyEvents.cpp
  texture_converter.cpp
  automateCCode.cpp
  ccodeWriteWrap.cpp
)
if(WIN32)
  list(APPEND common_SOURCES
    include/tools_windows.h
    tools_windows.cpp
  )
endif()

add_library(common ${common_SOURCES})

add_definitions(-DGITS_COMMON_PROJ)

target_link_libraries(common OpenGL_common xxhash murmurhash Boost::regex)

if(UNIX)
  target_link_libraries(common rt dl lua)
endif()

set(common_CCode_SOURCES
  config.cpp
  texture_converter.cpp
  timer.cpp
  log.cpp
  bit_range.cpp
  message_pump.cpp
  tools_lite.cpp
  tools.cpp
  resource_manager.cpp
  mapped_file.cpp
  exception.cpp
  dynamic_linker.cpp
  keyEvents.cpp
  getopt.cpp)
if(WIN32)
  list(APPEND common_CCode_SOURCES tools_windows.cpp)
endif()

set(common_CCode_HEADERS
  include/timer.h
  include/config.h
  include/message_pump.h
  include/tools_lite.h
  include/tools.h
  include/resource_manager.h
  include/mapped_file.h
  include/exception.h
  include/dynamic_linker.h
  include/key_value.h
  include/log.h
  include/malloc_allocator.h
  include/texture_converter.h
  include/platform.h
  include/pragmas.h
  include/bit_range.h
  include/keyEvents.h
  include/getopt_.h)
if(WIN32)
  list(APPEND common_CCode_HEADERS include/tools_windows.h)
endif()

install(FILES ${common_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${common_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)
