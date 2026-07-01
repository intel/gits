# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_api_debug STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LOGVKERROR_SRC
  ${SRC_DIR}/logVkErrorLayerAuto.h
  ${SRC_DIR}/logVkErrorLayerAuto.cpp
)
source_group("log_vk_error" FILES ${LOGVKERROR_SRC})

set(ALL_SRC
  ${LOGVKERROR_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_api_debug PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_api_debug PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(Vulkan_api_debug Vulkan_codegen)

target_link_libraries(Vulkan_api_debug PRIVATE
  Vulkan_layer_interface
  Vulkan_trace
)

set_target_properties(Vulkan_api_debug PROPERTIES FOLDER Vulkan/Vulkan_layers)
