# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OPENGL_INTERCEPTOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL_INTERCEPTOR_HEADER_DIR ${OPENGL_INTERCEPTOR_SOURCE_DIR}/include)

set_source_files_properties(
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/GLIPlugin64.def
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/GLIPlugin32.def
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/gitsPluginPrePostAuto.cpp
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/include/gitsPluginPrePostAuto.h
  PROPERTIES GENERATED 1
)

set(OpenGLLegacy_DEF )
if(MSVC)
  if("${CMAKE_GENERATOR_PLATFORM}" MATCHES "Win32")
    set(OpenGLLegacy_DEF ${OPENGL_INTERCEPTOR_SOURCE_DIR}/GLIPlugin32.def)
  elseif("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64")
    set(OpenGLLegacy_DEF ${OPENGL_INTERCEPTOR_SOURCE_DIR}/GLIPlugin64.def)
  else()
    message(STATUS "Unknown generator platform, treating it as x64. "
                    "(OpenGL interceptor will not export EGL calls.)")
    set(OpenGLLegacy_DEF ${OPENGL_INTERCEPTOR_SOURCE_DIR}/GLIPlugin64.def)
  endif()
endif()

list(APPEND OpenGLLegacy_interceptor_external_cpps
  ../../common/legacy/tools_lite.cpp
  ../../common/legacy/exception.cpp
  ../../common/legacy/timer.cpp
  ../../common/legacy/dynamic_linker.cpp
  ../../common/legacy/gitsLoader.cpp
)
if(WIN32)
  list(APPEND OpenGLLegacy_interceptor_external_cpps ../../common/legacy/tools_windows.cpp)
endif()

add_library(OpenGLLegacy_interceptor SHARED
  ${OPENGL_INTERCEPTOR_HEADER_DIR}/functionMap.h
  ${OPENGL_INTERCEPTOR_HEADER_DIR}/gitsPlugin.h
  ${OPENGL_INTERCEPTOR_HEADER_DIR}/gitsPluginPrePost.h
  ${OPENGL_INTERCEPTOR_HEADER_DIR}/gitsPluginPrePostAuto.h
  ${OPENGL_INTERCEPTOR_HEADER_DIR}/openglInterceptorExecOverride.h

  ${OpenGLLegacy_interceptor_external_cpps}
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/gitsPlugin.cpp
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/gitsPluginPrePost.cpp
  ${OPENGL_INTERCEPTOR_SOURCE_DIR}/gitsPluginPrePostAuto.cpp
  ${OpenGLLegacy_DEF}
)

if(WIN32)
    target_compile_definitions(OpenGLLegacy_interceptor PRIVATE PLOG_IMPORT)
else()
    target_compile_definitions(OpenGLLegacy_interceptor PRIVATE PLOG_GLOBAL)
endif()

target_include_directories(OpenGLLegacy_interceptor PRIVATE ${OPENGL_INTERCEPTOR_HEADER_DIR})

source_group("External Files" FILES ${OpenGLLegacy_interceptor_external_cpps})
source_group("Generated Files" FILES ${OpenGLLegacy_DEF} REGULAR_EXPRESSION ".*Auto.*")

target_link_libraries(OpenGLLegacy_interceptor PRIVATE gits_recorder_interface gits_recorder OpenGLLegacy_recorder_interface common_interface configuration log)
if(WIN32)
    target_link_options(OpenGLLegacy_interceptor PRIVATE "/DELAYLOAD:gitsRecorder.dll")
endif()

if(NOT MSVC)
  target_link_libraries(OpenGLLegacy_interceptor PRIVATE pthread dl)
endif()

if(MSVC)
  set_target_properties(OpenGLLegacy_interceptor PROPERTIES OUTPUT_NAME OpenGL32)
else()
  set_target_properties(OpenGLLegacy_interceptor PROPERTIES OUTPUT_NAME GitsGLCapture)
endif()

if(NOT MSVC)
  target_link_libraries(OpenGLLegacy_interceptor PRIVATE "-Wl,-Bsymbolic")
endif()

set_target_properties(OpenGLLegacy_interceptor PROPERTIES FOLDER OpenGLLegacy)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGLLegacy_interceptor> DESTINATION Recorder/${OpenGLLegacy_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_PDB_FILE:OpenGLLegacy_interceptor> DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS OpenGLLegacy_interceptor
  RUNTIME DESTINATION Recorder/${OpenGLLegacy_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${OpenGLLegacy_INSTALL_SUBDIR})
if(WITH_OPENCL)
  install(TARGETS OpenGLLegacy_interceptor
    RUNTIME DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR}
    LIBRARY DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR})
endif()
if(WIN32)
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:OpenGLLegacy_interceptor> RENAME libEGL.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:OpenGLLegacy_interceptor> RENAME libGLESv1_CM.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:OpenGLLegacy_interceptor> RENAME libGLESv2.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  endif()
  install(FILES $<TARGET_FILE:OpenGLLegacy_interceptor> RENAME libEGL.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_FILE:OpenGLLegacy_interceptor> RENAME libGLESv1_CM.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_FILE:OpenGLLegacy_interceptor> RENAME libGLESv2.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
endif()
