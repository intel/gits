# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Example for module authors. The build-tree Studio runs with it, but it is never installed.
if(WIN64)
  add_library(hello_module SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/helloPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helloPanel.cpp
  )

  gui_module(hello_module hello)
endif()
