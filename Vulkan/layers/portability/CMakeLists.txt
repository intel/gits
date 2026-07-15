# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_portability STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PORTABILITY_SRC
  ${SRC_DIR}/portabilityLayer.h
  ${SRC_DIR}/portabilityLayer.cpp
)
source_group("" FILES ${PORTABILITY_SRC})

target_sources(Vulkan_portability PRIVATE
  ${PORTABILITY_SRC}
)

target_link_libraries(Vulkan_portability PRIVATE
  Vulkan_layer_interface
)

target_include_directories(Vulkan_portability PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(Vulkan_portability PROPERTIES FOLDER Vulkan/Vulkan_layers)
