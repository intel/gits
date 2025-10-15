# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_HEADER_DIR ${COMMON_SOURCE_DIR}/include)

list(APPEND common_SOURCES
  ${COMMON_HEADER_DIR}/apis_iface.h
  ${COMMON_HEADER_DIR}/argument.h
  ${COMMON_HEADER_DIR}/bit_range.h
  ${COMMON_HEADER_DIR}/buffer.h
  ${COMMON_HEADER_DIR}/configUtils.h
  ${COMMON_HEADER_DIR}/diagnostic.h
  ${COMMON_HEADER_DIR}/dynamic_linker.h
  ${COMMON_HEADER_DIR}/exception.h
  ${COMMON_HEADER_DIR}/function.h
  ${COMMON_HEADER_DIR}/getopt_.h
  ${COMMON_HEADER_DIR}/gits.h
  ${COMMON_HEADER_DIR}/id.h
  ${COMMON_HEADER_DIR}/InputListener.h
  ${COMMON_HEADER_DIR}/istdhash.h
  ${COMMON_HEADER_DIR}/keyEvents.h
  ${COMMON_HEADER_DIR}/key_value.h
  ${COMMON_HEADER_DIR}/library.h
  ${COMMON_HEADER_DIR}/log.h
  ${COMMON_HEADER_DIR}/lua_bindings.h
  ${COMMON_HEADER_DIR}/macros.h
  ${COMMON_HEADER_DIR}/malloc_allocator.h
  ${COMMON_HEADER_DIR}/MemorySniffer.h
  ${COMMON_HEADER_DIR}/message_pump.h
  ${COMMON_HEADER_DIR}/messageBus.h
  ${COMMON_HEADER_DIR}/performance.h
  ${COMMON_HEADER_DIR}/platform.h
  ${COMMON_HEADER_DIR}/pragmas.h
  ${COMMON_HEADER_DIR}/resource_manager.h
  ${COMMON_HEADER_DIR}/runner.h
  ${COMMON_HEADER_DIR}/scheduler.h
  ${COMMON_HEADER_DIR}/streams.h
  ${COMMON_HEADER_DIR}/texture_converter.h
  ${COMMON_HEADER_DIR}/timer.h
  ${COMMON_HEADER_DIR}/token.h
  ${COMMON_HEADER_DIR}/tools_lite.h
  ${COMMON_HEADER_DIR}/tools.h
  ${COMMON_HEADER_DIR}/version.h
  ${COMMON_HEADER_DIR}/automateCCode.h
  ${COMMON_HEADER_DIR}/ccodeWriteWrap.h
  ${COMMON_HEADER_DIR}/intervalSet.h
  ${COMMON_HEADER_DIR}/controlHandler.h
  ${COMMON_HEADER_DIR}/recorderBehaviors.h
  ${COMMON_HEADER_DIR}/recorder.h
  ${COMMON_HEADER_DIR}/state.h
  ${COMMON_HEADER_DIR}/vectorMapper.h

  ${COMMON_SOURCE_DIR}/apis_iface.cpp
  ${COMMON_SOURCE_DIR}/argument.cpp
  ${COMMON_SOURCE_DIR}/bit_range.cpp
  ${COMMON_SOURCE_DIR}/buffer.cpp
  ${COMMON_SOURCE_DIR}/configUtils.cpp
  ${COMMON_SOURCE_DIR}/diagnostic.cpp
  ${COMMON_SOURCE_DIR}/exception.cpp
  ${COMMON_SOURCE_DIR}/function.cpp
  ${COMMON_SOURCE_DIR}/gits.cpp
  ${COMMON_SOURCE_DIR}/getopt.cpp
  ${COMMON_SOURCE_DIR}/id.cpp
  ${COMMON_SOURCE_DIR}/library.cpp
  ${COMMON_SOURCE_DIR}/log.cpp
  ${COMMON_SOURCE_DIR}/MemorySniffer.cpp
  ${COMMON_SOURCE_DIR}/message_pump.cpp
  ${COMMON_SOURCE_DIR}/messageBus.cpp
  ${COMMON_SOURCE_DIR}/performance.cpp
  ${COMMON_SOURCE_DIR}/resource_manager.cpp
  ${COMMON_SOURCE_DIR}/runner.cpp
  ${COMMON_SOURCE_DIR}/scheduler.cpp
  ${COMMON_SOURCE_DIR}/streams.cpp
  ${COMMON_SOURCE_DIR}/timer.cpp
  ${COMMON_SOURCE_DIR}/token.cpp
  ${COMMON_SOURCE_DIR}/tools_lite.cpp
  ${COMMON_SOURCE_DIR}/tools.cpp
  ${COMMON_SOURCE_DIR}/version.cpp
  ${COMMON_SOURCE_DIR}/dynamic_linker.cpp
  ${COMMON_SOURCE_DIR}/lua_bindings.cpp
  ${COMMON_SOURCE_DIR}/InputListener.cpp
  ${COMMON_SOURCE_DIR}/keyEvents.cpp
  ${COMMON_SOURCE_DIR}/texture_converter.cpp
  ${COMMON_SOURCE_DIR}/automateCCode.cpp
  ${COMMON_SOURCE_DIR}/ccodeWriteWrap.cpp
  ${COMMON_SOURCE_DIR}/controlHandler.cpp
  ${COMMON_SOURCE_DIR}/recorderBehaviors.cpp
  ${COMMON_SOURCE_DIR}/recorder.cpp
  ${COMMON_SOURCE_DIR}/state.cpp
)
source_group("recorder" FILES
  ${COMMON_HEADER_DIR}/controlHandler.h
  ${COMMON_HEADER_DIR}/recorderBehaviors.h
  ${COMMON_HEADER_DIR}/recorder.h
  ${COMMON_HEADER_DIR}/state.cpp

  ${COMMON_SOURCE_DIR}/controlHandler.cpp
  ${COMMON_SOURCE_DIR}/recorderBehaviors.cpp
  ${COMMON_SOURCE_DIR}/recorder.cpp
  ${COMMON_SOURCE_DIR}/state.cpp
)

if(WIN32)
  list(APPEND common_SOURCES
    ${COMMON_HEADER_DIR}/tools_windows.h
    ${COMMON_SOURCE_DIR}/tools_windows.cpp
  )
endif()

add_library(common ${common_SOURCES})
set_target_properties(common PROPERTIES FOLDER "common")

add_definitions(-DGITS_COMMON_PROJ)

add_dependencies(common config_codegen)
target_include_directories(common PUBLIC ${COMMON_HEADER_DIR})
target_link_libraries(common PRIVATE OpenGL_common xxhash murmurhash lz4 libzstd_static hud gits_recorder_interface png lua zlib)
target_link_libraries(common PUBLIC configuration log)

if(WITH_VULKAN)
  target_link_libraries(common PRIVATE Vulkan_common)
endif()

if(UNIX)
  target_link_libraries(common PRIVATE rt dl lua)
endif()

