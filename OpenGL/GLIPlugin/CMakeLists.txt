# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin64.def
  ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin32.def
  ${CMAKE_CURRENT_SOURCE_DIR}/gitsPluginPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/gitsPluginPrePostAuto.h
  PROPERTIES GENERATED 1
)

set(OpenGL_DEF )
if(MSVC)
  if("${CMAKE_GENERATOR_PLATFORM}" MATCHES "Win32")
    set(OpenGL_DEF ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin32.def)
  elseif("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64")
    set(OpenGL_DEF ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin64.def)
  else()
    message(STATUS "Unknown generator platform, treating it as x64. "
                    "(OpenGL interceptor will not export EGL calls.)")
    set(OpenGL_DEF ${CMAKE_CURRENT_SOURCE_DIR}/GLIPlugin64.def)
  endif()
endif()

list(APPEND OpenGL_interceptor_external_cpps
  ../../common/legacy/tools_lite.cpp
  ../../common/legacy/exception.cpp
  ../../common/legacy/timer.cpp
  ../../common/legacy/dynamic_linker.cpp
  ../../common/legacy/gitsLoader.cpp
  ../../common/legacy/log.cpp
)
if(WIN32)
  list(APPEND OpenGL_interceptor_external_cpps ../../common/legacy/tools_windows.cpp)
endif()

add_library(OpenGL_interceptor SHARED
  include/functionMap.h
  include/gitsPlugin.h
  include/gitsPluginPrePost.h
  include/gitsPluginPrePostAuto.h
  include/openglInterceptorExecOverride.h

  ${OpenGL_interceptor_external_cpps}
  gitsPlugin.cpp
  gitsPluginPrePost.cpp
  gitsPluginPrePostAuto.cpp
  ${OpenGL_DEF}
)

source_group("External Files" FILES ${OpenGL_interceptor_external_cpps})
source_group("Generated Files" FILES ${OpenGL_DEF} REGULAR_EXPRESSION ".*Auto.*")

if(NOT MSVC)
  target_link_libraries(OpenGL_interceptor pthread dl)
endif()

if(MSVC)
  set_target_properties(OpenGL_interceptor PROPERTIES OUTPUT_NAME OpenGL32)
else()
  set_target_properties(OpenGL_interceptor PROPERTIES OUTPUT_NAME GitsGLCapture)
endif()

if(NOT MSVC)
  target_link_libraries(OpenGL_interceptor "-Wl,-Bsymbolic")
endif()

set_target_properties(OpenGL_interceptor PROPERTIES FOLDER OpenGL)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS OpenGL_interceptor
  RUNTIME DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR})
if(WITH_OPENCL)
  install(TARGETS OpenGL_interceptor
    RUNTIME DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR}
    LIBRARY DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR})
endif()
if(WIN32)
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> RENAME libEGL.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> RENAME libGLESv1_CM.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> RENAME libGLESv2.pdb DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  endif()
  install(FILES $<TARGET_FILE:OpenGL_interceptor> RENAME libEGL.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_FILE:OpenGL_interceptor> RENAME libGLESv1_CM.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
  install(FILES $<TARGET_FILE:OpenGL_interceptor> RENAME libGLESv2.dll DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR} OPTIONAL)
endif()
