# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_layer_interface STATIC)

set(ALL_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanHeader2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandIdsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/layerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/command.h
  ${CMAKE_CURRENT_SOURCE_DIR}/arguments.h
  ${CMAKE_CURRENT_SOURCE_DIR}/arguments.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandsCustom.h
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_layer_interface PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_layer_interface PUBLIC
  ${VULKAN_HEADERS_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(Vulkan_layer_interface
  Vulkan_codegen_2
  config_codegen
)

target_link_libraries(Vulkan_layer_interface PUBLIC
  common
)

set_target_properties(Vulkan_layer_interface PROPERTIES FOLDER Vulkan2/Vulkan_common)
