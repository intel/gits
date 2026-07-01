# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_utils STATIC)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Vulkan_utils
PRIVATE
  ${SRC_DIR}/pluginService.h
  ${SRC_DIR}/pluginService.cpp
)

target_include_directories(Vulkan_utils
PUBLIC
  ${SRC_DIR}
  ${CMAKE_SOURCE_DIR}/plugins
)

target_link_libraries(Vulkan_utils
PUBLIC
  configuration
  common
)

add_dependencies(Vulkan_utils config_codegen)

set_target_properties(Vulkan_utils PROPERTIES FOLDER Vulkan/Vulkan_common)
