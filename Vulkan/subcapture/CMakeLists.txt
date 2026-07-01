# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_subcapture STATIC)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(STATE_TRACKING_SRC
  ${SRC_DIR}/objectState.h
  ${SRC_DIR}/stateTrackingService.h
  ${SRC_DIR}/stateTrackingService.cpp
  ${SRC_DIR}/descriptorSetUpdateService.h
  ${SRC_DIR}/descriptorSetUpdateService.cpp
  ${SRC_DIR}/syncStateService.h
  ${SRC_DIR}/syncStateService.cpp
  ${SRC_DIR}/queryPoolStateService.h
  ${SRC_DIR}/queryPoolStateService.cpp
  ${SRC_DIR}/imageLayoutService.h
  ${SRC_DIR}/imageLayoutService.cpp
  ${SRC_DIR}/commandBufferLifecycleService.h
  ${SRC_DIR}/commandBufferLifecycleService.cpp
  ${SRC_DIR}/mappedMemoryService.h
  ${SRC_DIR}/mappedMemoryService.cpp
  ${SRC_DIR}/subcaptureRange.h
  ${SRC_DIR}/subcaptureRange.cpp
  ${SRC_DIR}/subcaptureRecorder.h
  ${SRC_DIR}/subcaptureRecorder.cpp
  ${SRC_DIR}/analyzerResults.h
  ${SRC_DIR}/analyzerResults.cpp
  ${SRC_DIR}/analyzerService.h
  ${SRC_DIR}/analyzerService.cpp
)
source_group("state_tracking" FILES ${STATE_TRACKING_SRC})

set(LAYER_SRC
  ${SRC_DIR}/subcaptureLayer.h
  ${SRC_DIR}/subcaptureLayer.cpp
  ${SRC_DIR}/recordingLayerAuto.h
  ${SRC_DIR}/recordingLayerAuto.cpp
  ${SRC_DIR}/recordingLayerCustom.cpp
  ${SRC_DIR}/analyzerLayerAuto.h
  ${SRC_DIR}/analyzerLayerAuto.cpp
  ${SRC_DIR}/gpuReadbackHelper.h
  ${SRC_DIR}/gpuReadbackHelper.cpp
)
source_group("layer" FILES ${LAYER_SRC})

set(ALL_SRC
  ${STATE_TRACKING_SRC}
  ${LAYER_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_subcapture PRIVATE
  ${ALL_SRC}
)

add_dependencies(Vulkan_subcapture Vulkan_codegen)

target_include_directories(Vulkan_subcapture PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# gpuReadbackHelper / SubcaptureLayer use PlayerManager dispatch tables; headers live
# under Vulkan/player.  playerLayerManager.h additionally pulls in traceLayerGroup.h
# and resourceDumpingLayerGroup.h, so their directories must also be visible.
# PRIVATE so dependents are not forced to add these paths.
target_include_directories(Vulkan_subcapture PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../player
  ${VULKAN_DIR}/layers/trace
  ${VULKAN_DIR}/layers/resource_dumping
)

target_link_libraries(Vulkan_subcapture PRIVATE
  stream
  Vulkan_layer_interface
  Vulkan_coders
  Vulkan_services
  yamlcpp
)

set_target_properties(Vulkan_subcapture PROPERTIES FOLDER Vulkan)
