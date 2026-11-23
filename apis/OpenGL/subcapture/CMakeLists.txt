# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/recordingLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/recordingLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/analyzerLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/analyzerLayerAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL_subcapture STATIC
  ${GENERATED_SRC}
  analyzerLayerCustom.cpp
  analyzerResults.h
  analyzerResults.cpp
  liveGl.h
  recordingLayerCustom.cpp
  stateRestorer.h
  stateRestorer.cpp
  stateRestorerCompatibility.cpp
  stateRestorerContainers.cpp
  stateRestorerPrograms.cpp
  stateRestorerResources.cpp
  stateTrackingLayer.h
  stateTrackingLayer.cpp
  stateTrackingService.h
  stateTrackingService.cpp
  subcaptureLayer.h
  subcaptureLayer.cpp
  subcaptureRange.h
  subcaptureRange.cpp
  subcaptureRecorder.h
  subcaptureRecorder.cpp
)
add_dependencies(OpenGL_subcapture OpenGL_codegen)
# State restore reads the replay state through the player's driver and object maps.
target_include_directories(OpenGL_subcapture
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ../player/include ../interceptor
)
target_link_libraries(OpenGL_subcapture
  PUBLIC OpenGL_layer_interface OpenGL_services stream
  PRIVATE log common_interface yamlcpp OpenGL_platform
)
set_target_properties(OpenGL_subcapture PROPERTIES FOLDER OpenGL)
