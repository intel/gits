# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_executable(traceIpc)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(traceIpc PRIVATE
  ${SRC_DIR}/sharedCircularBuffer.h
  ${SRC_DIR}/main.cpp
)

set_target_properties(traceIpc PROPERTIES FOLDER "common")

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:traceIpc> DESTINATION UtilityTools OPTIONAL)
endif()

install(TARGETS traceIpc DESTINATION UtilityTools)