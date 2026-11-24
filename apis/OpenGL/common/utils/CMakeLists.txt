# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL_utils STATIC
  glContextQueries.h
  glContextQueries.cpp
  glVersionString.h
  glVersionString.cpp
)

add_dependencies(OpenGL_utils OpenGL_codegen)
target_include_directories(OpenGL_utils
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  # Live queries use the generated declarations; each final binary supplies its own table.
  PRIVATE ../dispatch ../layer_interface ../../interceptor
)

set_target_properties(OpenGL_utils PROPERTIES
  FOLDER OpenGL/OpenGL_common
  POSITION_INDEPENDENT_CODE ON
)
