# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_executable(DirectX_trace_ipc)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(DirectX_trace_ipc PRIVATE
  ${SRC_DIR}/common.h
  ${SRC_DIR}/main.cpp
)

set_target_properties(DirectX_trace_ipc PROPERTIES FOLDER DirectX/DirectX_layers)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_trace_ipc> DESTINATION UtilityTools OPTIONAL)
endif()

install(TARGETS DirectX_trace_ipc DESTINATION UtilityTools)