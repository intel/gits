# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DETOURS_ROOT)
  install_dependencies("--with-detours")
  set(DETOURS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Detours/src")
endif()

add_library(detours STATIC)

target_sources(detours
PRIVATE
  ${DETOURS_ROOT}/detours.cpp
  ${DETOURS_ROOT}/detours.h
  ${DETOURS_ROOT}/disasm.cpp
  ${DETOURS_ROOT}/modules.cpp
)
set_target_properties(detours PROPERTIES FOLDER External)

include_directories(SYSTEM ${DETOURS_ROOT})
