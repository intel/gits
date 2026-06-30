# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_executable(traceIpcLinux)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(traceIpcLinux PRIVATE
  ${SRC_DIR}/sharedCircularBuffer.h
  ${SRC_DIR}/main.cpp
)

set_target_properties(traceIpcLinux PROPERTIES FOLDER "common")

if(UNIX)
  target_link_libraries(traceIpcLinux PRIVATE rt pthread)
endif()

install(TARGETS traceIpcLinux
  RUNTIME DESTINATION UtilityTools
)
