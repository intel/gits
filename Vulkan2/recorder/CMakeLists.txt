# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_recorder_2 STATIC)

set(ROOT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/captureManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/captureManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/captureLayerManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/captureLayerManager.cpp
)
source_group("" FILES ${ROOT_SRC})

set(CAPTURE_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanApiIfaceRecorder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanRecorderInterface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanRecorder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanRecorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dispatchTableAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wrappersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wrappersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/testLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/encoderLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/encoderLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gitsRecorder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gitsRecorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/captureCustomizationLayer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/captureCustomizationLayer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdaters.h
)
source_group("capture" FILES ${CAPTURE_SRC})

set(SERVICES_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/mapTrackingService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mapTrackingService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptorUpdateTemplateService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptorUpdateTemplateService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${CAPTURE_SRC}
  ${SERVICES_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_recorder_2 PRIVATE
  ${ALL_SRC}
)

target_link_libraries(Vulkan_recorder_2 PRIVATE
  gits_recorder_interface
  Vulkan_layer_interface
  Vulkan_services
  Vulkan_coders
)

set_target_properties(Vulkan_recorder_2 PROPERTIES FOLDER Vulkan2)
