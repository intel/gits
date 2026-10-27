# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OpenGLLegacy_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/OpenGLLegacy/codegen")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OpenGLLegacy_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "${OpenGLLegacy_SCRIPTS_DIR}/generate_gl.py"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/OpenGLLegacy_codegen.stamp"
  WORKING_DIRECTORY "${OpenGLLegacy_SCRIPTS_DIR}"
  DEPENDS
    "${OpenGLLegacy_SCRIPTS_DIR}/generate_gl.py"
    "${OpenGLLegacy_SCRIPTS_DIR}/generator.py"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/gitsPluginPrePostAuto.cpp.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/gitsPluginPrePostAuto.h.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glDrivers.h.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glEnumsAuto.h.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glEnumsAuto.cpp.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glFunctions.cpp.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glFunctions.h.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/glIDswitch.h.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/GLIPluginXX.def.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/openglRecorderWrapperAuto.cpp.mako"
    "${OpenGLLegacy_SCRIPTS_DIR}/templates/openglRecorderWrapperXAuto.h.mako"
)
add_custom_target(OpenGLLegacy_codegen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/OpenGLLegacy_codegen.stamp")
set_target_properties(OpenGLLegacy_codegen PROPERTIES FOLDER OpenGLLegacy)

# GENERATED property is set in CMakeLists.txt in each directory, but this
# staying here doesn't do any harm
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glDrivers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glEnumsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glIDs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/glIDswitch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/glEnumsAuto.cpp
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

add_dependencies(OpenGLLegacy_common OpenGLLegacy_codegen config_codegen)
add_dependencies(OpenGLLegacy_recorder OpenGLLegacy_codegen config_codegen)
add_dependencies(OpenGLLegacy_interceptor OpenGLLegacy_codegen config_codegen)
