# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DETOURS_ROOT)
  add_thirdparty_arg_setup("--with-detours" init_detours)
  set(DETOURS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Detours/src")
endif()

function(init_detours)
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
endfunction()