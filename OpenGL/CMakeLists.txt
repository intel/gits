# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OpenGL_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/OpenGL/codegen")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OpenGL_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "${OpenGL_SCRIPTS_DIR}/generate_gl.py"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/OpenGL_codegen.stamp"
  WORKING_DIRECTORY "${OpenGL_SCRIPTS_DIR}"
  DEPENDS
    "${OpenGL_SCRIPTS_DIR}/generate_gl.py"
    "${OpenGL_SCRIPTS_DIR}/generator.py"
)
add_custom_target(OpenGL_codegen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/OpenGL_codegen.stamp")
set_target_properties(OpenGL_codegen PROPERTIES FOLDER OpenGL)

# GENERATED property is set in CMakeLists.txt in each directory, but this
# staying here doesn't do any harm
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glDrivers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glIDs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glIDswitch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/glFunctions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/openglRecorderWrapperAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/include/openglRecorderWrapperAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/include/openglRecorderWrapperIfaceAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin/GLIPlugin64.def
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin/GLIPlugin32.def
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin/gitsPluginPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin/include/gitsPluginPrePostAuto.h
  PROPERTIES GENERATED 1
)

add_subdirectory(common)
add_subdirectory(GLIPlugin)
add_subdirectory(recorder)

add_dependencies(OpenGL_common OpenGL_codegen)
add_dependencies(OpenGL_recorder OpenGL_codegen)
add_dependencies(OpenGL_interceptor OpenGL_codegen)
