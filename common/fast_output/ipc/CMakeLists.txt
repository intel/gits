# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_executable(trace_ipc)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(trace_ipc PRIVATE
  ${SRC_DIR}/common.h
  ${SRC_DIR}/main.cpp
)

set_target_properties(trace_ipc PROPERTIES FOLDER common/trace)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:trace_ipc> DESTINATION UtilityTools OPTIONAL)
endif()

install(TARGETS trace_ipc DESTINATION UtilityTools)