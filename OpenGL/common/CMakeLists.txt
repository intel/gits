# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OPENGL_COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL_COMMON_HEADER_DIR ${OPENGL_COMMON_SOURCE_DIR}/include)

set_source_files_properties(
  ${OPENGL_COMMON_HEADER_DIR}/glDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/glFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDs.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDswitch.h
  ${OPENGL_COMMON_SOURCE_DIR}/glFunctions.cpp
  PROPERTIES GENERATED 1
)

add_library(OpenGL_common
  ${OPENGL_COMMON_HEADER_DIR}/clientArrays.h
  ${OPENGL_COMMON_HEADER_DIR}/eglArguments.h
  ${OPENGL_COMMON_HEADER_DIR}/eglFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/gitsFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/glDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/glFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDs.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDswitch.h
  ${OPENGL_COMMON_HEADER_DIR}/glxArguments.h
  ${OPENGL_COMMON_HEADER_DIR}/glxFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/mapping.h
  ${OPENGL_COMMON_HEADER_DIR}/nonglArguments.h
  ${OPENGL_COMMON_HEADER_DIR}/openglArguments.h
  ${OPENGL_COMMON_HEADER_DIR}/openglCCodeWriteWrap.h
  ${OPENGL_COMMON_HEADER_DIR}/openglCommon.h
  ${OPENGL_COMMON_HEADER_DIR}/openglDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/openglEnums.h
  ${OPENGL_COMMON_HEADER_DIR}/openglFunction.h
  ${OPENGL_COMMON_HEADER_DIR}/openglLibrary.h
  ${OPENGL_COMMON_HEADER_DIR}/openglTools.h
  ${OPENGL_COMMON_HEADER_DIR}/openglTypes.h
  ${OPENGL_COMMON_HEADER_DIR}/opengl_apis_iface.h
  ${OPENGL_COMMON_HEADER_DIR}/pfattribs.h
  ${OPENGL_COMMON_HEADER_DIR}/playerRunWrap.h
  ${OPENGL_COMMON_HEADER_DIR}/playerRunWrapConditions.h
  ${OPENGL_COMMON_HEADER_DIR}/ptblLibrary.h
  ${OPENGL_COMMON_HEADER_DIR}/ptbl_eglLibrary.h
  ${OPENGL_COMMON_HEADER_DIR}/ptbl_glxLibrary.h
  ${OPENGL_COMMON_HEADER_DIR}/ptbl_wglLibrary.h
  ${OPENGL_COMMON_HEADER_DIR}/stateDynamic.h
  ${OPENGL_COMMON_HEADER_DIR}/stateObjects.h
  ${OPENGL_COMMON_HEADER_DIR}/stateTracking.h
  ${OPENGL_COMMON_HEADER_DIR}/typeDefs.h
  ${OPENGL_COMMON_HEADER_DIR}/wglArguments.h
  ${OPENGL_COMMON_HEADER_DIR}/wglFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/windowContextState.h
  ${OPENGL_COMMON_HEADER_DIR}/windowing.h

  ${OPENGL_COMMON_SOURCE_DIR}/eglArguments.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/eglFunctions.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/glxArguments.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/glxFunctions.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/glFunctions.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/nonglArguments.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/mapping.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/opengl_apis_iface.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglArguments.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglCCodeWriteWrap.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglCommon.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglDrivers.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglFunction.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglLibrary.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglTools.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/pfattribs.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/stateDynamic.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/stateObjects.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/wglArguments.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/wglFunctions.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/windowContextState.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/windowing.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/glEnumStrings.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/clientArrays.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/gitsFunctions.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/stateTracking.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/ptblLibrary.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/ptbl_wglLibrary.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/ptbl_glxLibrary.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/ptbl_eglLibrary.cpp
)

target_include_directories(OpenGL_common PUBLIC ${OPENGL_COMMON_HEADER_DIR})

add_library(OpenGL_common_interface INTERFACE)
target_include_directories(OpenGL_common_interface INTERFACE ${OPENGL_COMMON_HEADER_DIR})

source_group("Generated Files" FILES
  ${OPENGL_COMMON_HEADER_DIR}/glDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/glFunctions.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDs.h
  ${OPENGL_COMMON_HEADER_DIR}/glIDswitch.h
  ${OPENGL_COMMON_SOURCE_DIR}/glFunctions.cpp)

target_link_libraries(OpenGL_common PRIVATE common configuration)
if (NOT MSVC)
  target_link_libraries(OpenGL_common PRIVATE X11 xcb X11-xcb wayland-client wayland-egl)
endif()

set_target_properties(OpenGL_common PROPERTIES FOLDER OpenGL)

set(OpenGL_CCode_SOURCES
  ${OPENGL_COMMON_SOURCE_DIR}/windowing.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/windowContextState.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/pfattribs.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/glEnumStrings.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglDrivers.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglTools.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/openglCommon.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/mapping.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/stateDynamic.cpp
  ${OPENGL_COMMON_SOURCE_DIR}/stateObjects.cpp)

set(OpenGL_CCode_HEADERS
  ${OPENGL_COMMON_HEADER_DIR}/windowing.h
  ${OPENGL_COMMON_HEADER_DIR}/windowContextState.h
  ${OPENGL_COMMON_HEADER_DIR}/pfattribs.h
  ${OPENGL_COMMON_HEADER_DIR}/openglTypes.h
  ${OPENGL_COMMON_HEADER_DIR}/openglEnums.h
  ${OPENGL_COMMON_HEADER_DIR}/openglDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/glDrivers.h
  ${OPENGL_COMMON_HEADER_DIR}/openglTools.h
  ${OPENGL_COMMON_HEADER_DIR}/typeDefs.h
  ${OPENGL_COMMON_HEADER_DIR}/openglCommon.h
  ${OPENGL_COMMON_HEADER_DIR}/mapping.h
  ${OPENGL_COMMON_HEADER_DIR}/stateDynamic.h
  ${OPENGL_COMMON_HEADER_DIR}/stateObjects.h)

install(FILES ${OpenGL_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${OpenGL_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)
