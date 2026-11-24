# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL_utils STATIC
  glVersionString.h
  glVersionString.cpp
)

target_include_directories(OpenGL_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(OpenGL_utils PROPERTIES
  FOLDER OpenGL/OpenGL_common
  POSITION_INDEPENDENT_CODE ON
)
