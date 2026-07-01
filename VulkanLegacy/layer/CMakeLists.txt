# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/../interceptor/vulkanPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vkLayer.def
  PROPERTIES GENERATED 1
)

set(VulkanLayer_DEF )
if(MSVC)
  set(VulkanLayer_DEF vkLayer.def)
endif()

add_definitions(-DBUILD_FOR_VULKAN_LAYER)

list(APPEND VulkanLegacy_layer_external_cpps
  ../../common/legacy/tools_lite.cpp
  ../../common/legacy/exception.cpp
  ../../common/legacy/dynamic_linker.cpp
  ../../common/legacy/gitsLoader.cpp
)
if(WIN32)
  list(APPEND VulkanLegacy_layer_external_cpps ../../common/legacy/tools_windows.cpp)
endif()

add_library(VulkanLegacy_layer SHARED
  ../interceptor/include/gitsPluginVulkan.h
  ../interceptor/include/vulkanExecWrap.h

  ${VulkanLegacy_layer_external_cpps}
  ../interceptor/include/gitsPluginVulkan.cpp
  ../interceptor/vulkanPrePostAuto.cpp
  ${VulkanLayer_DEF}
)

source_group("External Files" FILES ${VulkanLegacy_layer_external_cpps})
source_group("Generated Files" FILES ${VulkanLayer_DEF} REGULAR_EXPRESSION ".*Auto.*")

target_include_directories(VulkanLegacy_layer PRIVATE ${VULKAN_HEADERS_DIR})
target_link_libraries(VulkanLegacy_layer PRIVATE gits_recorder_interface common configuration)
if(WIN32)
  target_link_libraries(VulkanLegacy_layer PRIVATE hud)
endif()

if(MSVC)
  set_target_properties(VulkanLegacy_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
else()
  set_target_properties(VulkanLegacy_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
  target_link_libraries(VulkanLegacy_layer PRIVATE pthread dl)
endif()

set_target_properties(VulkanLegacy_layer PROPERTIES FOLDER VulkanLegacy)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:VulkanLegacy_layer> DESTINATION Recorder/${VulkanLayerLegacy_INSTALL_SUBDIR} OPTIONAL)
endif()
install(TARGETS VulkanLegacy_layer
  RUNTIME DESTINATION Recorder/${VulkanLayerLegacy_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLayerLegacy_INSTALL_SUBDIR})
