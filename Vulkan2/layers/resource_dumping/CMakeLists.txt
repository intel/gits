# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_resource_dumping STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/resourceDumpingLayerGroup.h
  ${SRC_DIR}/resourceDumpingLayerGroup.cpp
)
source_group("" FILES ${ROOT_SRC})

set(SCREENSHOTS_SRC
  ${SRC_DIR}/screenshotsLayer.h
  ${SRC_DIR}/screenshotsLayer.cpp
  ${SRC_DIR}/swapchainImagesDumper.h
  ${SRC_DIR}/swapchainImagesDumper.cpp
)
source_group("screenshots" FILES ${SCREENSHOTS_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${SCREENSHOTS_SRC}
)

set_auto_generated_property(FILE_LIST  ${ALL_SRC})

target_sources(Vulkan_resource_dumping PRIVATE
  ${ALL_SRC}
)

target_link_libraries(Vulkan_resource_dumping PRIVATE
  stb_implementation
  Vulkan_layer_interface
  Vulkan_utils
)

target_include_directories(Vulkan_resource_dumping PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(Vulkan_resource_dumping PRIVATE
  ${STB_DIR}
)

set_target_properties(Vulkan_resource_dumping PROPERTIES FOLDER Vulkan2/Vulkan_layers)
