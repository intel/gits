# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_HEADER_DIR ${COMMON_SOURCE_DIR}/include)

list(APPEND common_SOURCES
  ${COMMON_HEADER_DIR}/bit_range.h
  ${COMMON_HEADER_DIR}/configUtils.h
  ${COMMON_HEADER_DIR}/diagnostic.h
  ${COMMON_HEADER_DIR}/dynamic_linker.h
  ${COMMON_HEADER_DIR}/exception.h
  ${COMMON_HEADER_DIR}/gits.h
  ${COMMON_HEADER_DIR}/gitsVersion.h
  ${COMMON_HEADER_DIR}/ipcPipe.h
  ${COMMON_HEADER_DIR}/istdhash.h
  ${COMMON_HEADER_DIR}/keyEvents.h
  ${COMMON_HEADER_DIR}/messageBus.h
  ${COMMON_HEADER_DIR}/platform.h
  ${COMMON_HEADER_DIR}/streamFile.h
  ${COMMON_HEADER_DIR}/timer.h
  ${COMMON_HEADER_DIR}/tools_lite.h
  ${COMMON_HEADER_DIR}/tools.h
  ${COMMON_HEADER_DIR}/version.h

  ${COMMON_SOURCE_DIR}/bit_range.cpp
  ${COMMON_SOURCE_DIR}/configUtils.cpp
  ${COMMON_SOURCE_DIR}/diagnostic.cpp
  ${COMMON_SOURCE_DIR}/exception.cpp
  ${COMMON_SOURCE_DIR}/gits.cpp
  ${COMMON_SOURCE_DIR}/ipcPipe.cpp
  ${COMMON_SOURCE_DIR}/messageBus.cpp
  ${COMMON_SOURCE_DIR}/timer.cpp
  ${COMMON_SOURCE_DIR}/tools_lite.cpp
  ${COMMON_SOURCE_DIR}/tools.cpp
  ${COMMON_SOURCE_DIR}/version.cpp
  ${COMMON_SOURCE_DIR}/dynamic_linker.cpp
  ${COMMON_SOURCE_DIR}/keyEvents.cpp
)

if(WIN32)
  list(APPEND common_SOURCES
    ${COMMON_HEADER_DIR}/tools_windows.h
    ${COMMON_SOURCE_DIR}/tools_windows.cpp
  )
endif()

add_library(common ${common_SOURCES})
set_target_properties(common PROPERTIES FOLDER "common")

add_dependencies(common config_codegen gits_build_info)
target_include_directories(common PUBLIC ${COMMON_HEADER_DIR})
target_include_directories(common PRIVATE ${GITS_BUILD_INFO_INCLUDE_DIR})
target_link_libraries(common PRIVATE xxhash hud gits_recorder_interface)
target_link_libraries(common PUBLIC configuration log)

if(UNIX)
  target_link_libraries(common PRIVATE rt dl X11)
endif()

add_library(common_interface INTERFACE)
target_include_directories(common_interface INTERFACE ${COMMON_HEADER_DIR})

# Host-only gate for the message bus: unsubscribing while a topic is published, publishing and
# subscribing from callbacks, and publishing from several threads. Run the executable; it prints
# failures and exits non-zero.
add_executable(common_message_bus_test
  ${COMMON_SOURCE_DIR}/tests/messageBusTest.cpp
)
target_link_libraries(common_message_bus_test PRIVATE common)
set_target_properties(common_message_bus_test PROPERTIES FOLDER "common")
