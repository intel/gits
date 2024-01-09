# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.13)
project(gits)
find_package(Python3 COMPONENTS Interpreter)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  add_compile_options(/MP /bigobj /W3 /wd4005 /wd4065 /wd4100 /wd4480 /wd4481 /wd4482 /wd4503 /wd4505 /wd4611 /wd4702 /wd4800 /wd4838 /wd4996 /wd4267 /wd4067 /wd4312 /wd4722 /wd4018 /wd4305)
  add_compile_options("/MT$<$<CONFIG:Debug>:d>")
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_SHARED_PTR_UNIQUE_DEPRECATION_WARNING)
elseif(UNIX)
  add_definitions(-Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings -Wno-format -fno-strict-aliasing -fPIC)
endif()

add_definitions(-DBUILD_FOR_CCODE)

if(EMPTY_BUILD)
  add_definitions(-D_EMPTY_BUILD)
endif()

include_directories(CCodeProject CCodeSource/StreamFiles CCodeSource/include CCodeSource/imports)

include(custom_functions)

include(import_boost)
include(import_zlib)
include(import_libpng)
include(import_xxhash)
include(import_clheaders)
include(import_murmurhash)

FILE(GLOB CCodeProject_SOURCES
  ${PROJECT_SOURCE_DIR}/CCodeSource/StreamFiles/*.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/bit_range.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/config.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/exception.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/getopt.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/gits.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/glEnumStrings.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/log.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/mapped_file.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/message_pump.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/openclDrivers.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/l0DriversInit.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/l0Drivers.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/l0Log.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/openglDrivers.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/openglTools.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/openglCommon.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/pfattribs.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/resource_manager.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/texture_converter.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/timer.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/tools_lite.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/tools.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/windowContextState.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/windowing.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/dynamic_linker.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/stateDynamic.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/stateObjects.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vkWindowing.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanDrivers.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanLog.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanLogAuto.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanTracerAuto.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanTools.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/keyEvents.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/vulkanTools_lite.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/main.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/helper.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/helperGL.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/helperCL.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/helperL0.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/helperVk.cpp
  ${PROJECT_SOURCE_DIR}/CCodeSource/mapping.cpp
)
if(WIN32)
  list(APPEND CCodeProject_SOURCES ${PROJECT_SOURCE_DIR}/CCodeSource/tools_windows.cpp)
endif()

add_executable(CCodeProject ${CCodeProject_SOURCES})
if(MSVC)
  target_link_options(CCodeProject PUBLIC /STACK:100000000 /HEAP:100000000 /ignore:4006)
  set(LIBS OpenGL32 SetupAPI zlib png xxhash murmurhash)
elseif(UNIX)
  set(LIBS GL X11 xcb X11-xcb wayland-client wayland-egl dl png pthread z xxhash murmurhash)
  set_target_properties(CCodeProject PROPERTIES LINK_FLAGS -pie)
endif()
target_link_libraries(CCodeProject ${LIBS})
