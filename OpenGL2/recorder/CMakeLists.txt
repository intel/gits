# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openglDriversAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/openglDriversAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recorderWrappersAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL2_recorder STATIC
  ${GENERATED_SRC}
  include/realDriver.h
  include/recorderInterface.h
  include/recursionGuard.h
  include/recorderWrappers.h
  include/unsupportedCommand.h
  realDriver.cpp
  recorder.cpp
  recursionGuard.cpp
  unsupportedCommand.cpp
)

add_dependencies(OpenGL2_recorder OpenGL2_codegen)

target_include_directories(OpenGL2_recorder
  PUBLIC include
  PRIVATE ../interceptor
)

target_link_libraries(OpenGL2_recorder PRIVATE log)

set_target_properties(OpenGL2_recorder PROPERTIES FOLDER OpenGL2)