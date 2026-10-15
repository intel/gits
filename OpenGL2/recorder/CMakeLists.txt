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
  ${CMAKE_CURRENT_SOURCE_DIR}/encoderLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/encoderLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recorderWrappersAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL2_recorder STATIC
  ${GENERATED_SRC}
  include/captureManager.h
  include/clientMemoryCapture.h
  include/handleArgumentUpdaters.h
  include/mapTrackingService.h
  include/mapTrackingWrappers.h
  include/pixelFormatCaptureLayer.h
  include/realDriver.h
  include/recorderInterface.h
  include/recursionGuard.h
  include/recorderWrappers.h
  include/unsupportedCommand.h
  include/windowGeometryTrackingLayer.h
  include/windowGeometryTrackingService.h
  captureManager.cpp
  clientMemoryCapture.cpp
  mapTrackingService.cpp
  mapTrackingWrappers.cpp
  pixelFormatCaptureLayer.cpp
  realDriver.cpp
  recorder.cpp
  recursionGuard.cpp
  unsupportedCommand.cpp
  windowGeometryTrackingLayer.cpp
  windowGeometryTrackingService.cpp
)

add_dependencies(OpenGL2_recorder OpenGL2_codegen)

target_include_directories(OpenGL2_recorder
  PUBLIC include
  PRIVATE ../interceptor
)

target_link_libraries(OpenGL2_recorder
  PUBLIC OpenGL2_layer_interface
  # gdi32: PixelFormatCaptureLayer calls the real GetPixelFormat()/DescribePixelFormat() GDI
  # exports directly (see pixelFormatCaptureLayer.cpp) - mirrors OpenGL2_player's identical
  # gdi32 dependency for HdcVirtualizationService's own ChoosePixelFormat/SetPixelFormat/
  # GetPixelFormat calls (player/CMakeLists.txt).
  # user32: windowGeometryTrackingService.cpp calls the real WindowFromDC()/GetClientRect()/
  # GetWindowRect()/IsWindowVisible() User32 exports directly - declared explicitly rather
  # than relying on some other dependency injecting it implicitly, mirroring
  # common/windowing/CMakeLists.txt's own explicit `user32` link on Win32.
  PRIVATE OpenGL2_services log tbb gdi32 user32
)

set_target_properties(OpenGL2_recorder PROPERTIES FOLDER OpenGL2)