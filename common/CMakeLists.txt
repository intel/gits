# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
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
  include/configTypes.h
  include/configYamlTemplates.h
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
  include/MemorySniffer.h
  include/message_pump.h
  include/messageBus.h
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
  include/automateCCode.h
  include/ccodeWriteWrap.h
  include/intervalSet.h
  include/controlHandler.h
  include/gitsLoader.h
  include/recorderBehaviors.h
  include/recorder.h
  include/state.h
  include/vectorMapper.h

  apis_iface.cpp
  argument.cpp
  bit_range.cpp
  buffer.cpp
  config.cpp
  configUtils.cpp
  diagnostic.cpp
  exception.cpp
  function.cpp
  gits.cpp
  getopt.cpp
  hashing.cpp
  id.cpp
  library.cpp
  log.cpp
  MemorySniffer.cpp
  message_pump.cpp
  messageBus.cpp
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
  lua_bindings.cpp
  InputListener.cpp
  keyEvents.cpp
  texture_converter.cpp
  automateCCode.cpp
  ccodeWriteWrap.cpp
  controlHandler.cpp
  gitsLoader.cpp
  recorderBehaviors.cpp
  recorder.cpp
  state.cpp
)
source_group("recorder" FILES
  include/controlHandler.h
  include/gitsLoader.h
  include/recorderBehaviors.h
  include/recorder.h
  include/state.cpp

  controlHandler.cpp
  gitsLoader.cpp
  recorderBehaviors.cpp
  recorder.cpp
  state.cpp
)

if(WIN32)
  list(APPEND common_SOURCES
    include/tools_windows.h
    include/eventHandlers.h
    tools_windows.cpp
    eventHandlers.cpp
  )
endif()

add_library(common ${common_SOURCES})

add_definitions(-DGITS_COMMON_PROJ)

target_link_libraries(common OpenGL_common xxhash murmurhash lz4 libzstd_static)

if(WITH_VULKAN)
  target_link_libraries(common Vulkan_common)
endif()

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
  exception.cpp
  dynamic_linker.cpp
  keyEvents.cpp
  getopt.cpp
  streams.cpp)
if(WIN32)
  list(APPEND common_CCode_SOURCES tools_windows.cpp)
endif()

set(common_CCode_HEADERS
  include/timer.h
  include/config.h
  include/configTypes.h
  include/message_pump.h
  include/tools_lite.h
  include/tools.h
  include/resource_manager.h
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
  include/getopt_.h
  include/streams.h
  include/intervalSet.h
  include/vectorMapper.h)
if(WIN32)
  list(APPEND common_CCode_HEADERS include/tools_windows.h)
endif()

install(FILES ${common_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${common_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)
