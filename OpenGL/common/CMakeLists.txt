# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glIDs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glIDswitch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glDrivers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/glFunctions.cpp
  PROPERTIES GENERATED 1
)

add_library(OpenGL_common
  include/glIDs.h
  include/glIDswitch.h
  include/clientArrays.h
  include/eglArguments.h
  include/eglFunctions.h
  include/gitsFunctions.h
  include/glDrivers.h
  include/glFunctions.h
  include/glxArguments.h
  include/glxFunctions.h
  include/mapping.h
  include/nonglArguments.h
  include/openglArguments.h
  include/openglCCodeWriteWrap.h
  include/openglCommon.h
  include/openglDrivers.h
  include/openglEnums.h
  include/openglFunction.h
  include/openglLibrary.h
  include/openglTools.h
  include/openglTypes.h
  include/opengl_apis_iface.h
  include/pfattribs.h
  include/playerRunWrap.h
  include/playerRunWrapConditions.h
  include/ptblLibrary.h
  include/ptbl_eglLibrary.h
  include/ptbl_glxLibrary.h
  include/ptbl_wglLibrary.h
  include/stateDynamic.h
  include/stateObjects.h
  include/stateTracking.h
  include/typeDefs.h
  include/wglArguments.h
  include/wglFunctions.h
  include/windowContextState.h
  include/windowing.h

  eglArguments.cpp
  eglFunctions.cpp
  glxArguments.cpp
  glxFunctions.cpp
  glFunctions.cpp
  nonglArguments.cpp 
  mapping.cpp
  opengl_apis_iface.cpp
  openglArguments.cpp
  openglCCodeWriteWrap.cpp
  openglCommon.cpp
  openglDrivers.cpp
  openglFunction.cpp
  openglLibrary.cpp
  openglTools.cpp
  pfattribs.cpp
  stateDynamic.cpp
  stateObjects.cpp
  wglArguments.cpp
  wglFunctions.cpp
  windowContextState.cpp
  windowing.cpp
  glEnumStrings.cpp
  clientArrays.cpp
  gitsFunctions.cpp
  stateTracking.cpp
  ptblLibrary.cpp
  ptbl_wglLibrary.cpp
  ptbl_glxLibrary.cpp
  ptbl_eglLibrary.cpp
)

source_group("Generated Files" FILES
  include/glIDs.h
  include/glIDswitch.h
  include/glDrivers.h
  include/glFunctions.h
  glFunctions.cpp)

target_link_libraries(OpenGL_common common)
if (NOT MSVC)
  target_link_libraries(OpenGL_common X11 xcb X11-xcb wayland-client wayland-egl)
endif()

set_target_properties(OpenGL_common PROPERTIES FOLDER OpenGL)

set(OpenGL_CCode_SOURCES
  windowing.cpp
  windowContextState.cpp
  pfattribs.cpp
  glEnumStrings.cpp
  openglDrivers.cpp
  openglTools.cpp
  openglCommon.cpp
  mapping.cpp
  stateDynamic.cpp
  stateObjects.cpp)

set(OpenGL_CCode_HEADERS
  include/windowing.h
  include/windowContextState.h
  include/pfattribs.h
  include/openglTypes.h
  include/openglEnums.h
  include/openglDrivers.h
  include/glDrivers.h
  include/openglTools.h
  include/typeDefs.h
  include/openglCommon.h
  include/mapping.h
  include/stateDynamic.h
  include/stateObjects.h)

install(FILES ${OpenGL_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${OpenGL_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)
