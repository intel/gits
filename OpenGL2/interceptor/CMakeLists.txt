# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptorAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptorAuto.def
  ${CMAKE_CURRENT_SOURCE_DIR}/openglDriversAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/openglDriversAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

set(HAND_WRITTEN_SRC
  glTypes.h
  logging.h
  logging.cpp
  realDriver.h
  realDriver.cpp
  interceptorCustom.cpp
)

add_library(OpenGL2_interceptor SHARED
  ${GENERATED_SRC}
  ${HAND_WRITTEN_SRC}
)

add_dependencies(OpenGL2_interceptor OpenGL2_codegen)

target_link_libraries(OpenGL2_interceptor PRIVATE log)

set_target_properties(OpenGL2_interceptor PROPERTIES
  OUTPUT_NAME OpenGL32
  FOLDER OpenGL2
)
