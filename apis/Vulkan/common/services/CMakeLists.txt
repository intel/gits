# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_services STATIC)

set(SERVICES_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/handleMapService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/handleMapService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

set_auto_generated_property(FILE_LIST ${SERVICES_SRC})

set(ALL_SRC
  ${SERVICES_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_services PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_services PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_services PRIVATE
  Vulkan_layer_interface
)

set_target_properties(Vulkan_services PROPERTIES FOLDER Vulkan/Vulkan_common)
