# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/OpenGL2_codegen.stamp)

add_custom_target(OpenGL2_codegen DEPENDS ${STAMP_FILE})
set_target_properties(OpenGL2_codegen PROPERTIES FOLDER OpenGL2)

set(SCRIPTS_SRC
  command_ids.py
  intermediates.py
  intermediates_creator.py
  generator_helpers.py
  generator.py
  generator_interceptor.py
  xml_data_exploration.py
  registry.py
)
source_group("scripts" FILES ${SCRIPTS_SRC})

set(PERSISTENT_DATA_SRC
  command_ids.json
)
source_group("persistent data" FILES ${PERSISTENT_DATA_SRC})

file(GLOB_RECURSE MAKO_TEMPLATES "templates/*.mako")
source_group(mako FILES ${MAKO_TEMPLATES})

target_sources(OpenGL2_codegen PRIVATE
  ${MAKO_TEMPLATES}
  ${PERSISTENT_DATA_SRC}
  ${SCRIPTS_SRC}
)

set(OPENGL2_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL2_INTERCEPTOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../interceptor)
set(OPENGL2_GENERATED_INTERCEPTOR_FILES
  ${OPENGL2_INTERCEPTOR_DIR}/interceptorAuto.cpp
  ${OPENGL2_INTERCEPTOR_DIR}/openglDriversAuto.h
  ${OPENGL2_INTERCEPTOR_DIR}/openglDriversAuto.cpp
  ${OPENGL2_INTERCEPTOR_DIR}/interceptorAuto.def
)

add_custom_command(
  OUTPUT ${STAMP_FILE} ${OPENGL2_GENERATED_INTERCEPTOR_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Running OpenGL2 code generation script..."
  COMMAND ${Python3_EXECUTABLE} ${OPENGL2_SCRIPTS_DIR}/generator.py
  COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
  WORKING_DIRECTORY ${OPENGL2_SCRIPTS_DIR}
  DEPENDS ${SCRIPTS_SRC} ${PERSISTENT_DATA_SRC} ${MAKO_TEMPLATES}
)

unset(OpenGL2_codegen CACHE)
