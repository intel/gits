# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
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

list(APPEND Vulkan_layer_external_cpps
  ../../common/legacy/tools_lite.cpp
  ../../common/legacy/exception.cpp
  ../../common/legacy/dynamic_linker.cpp
  ../../common/legacy/gitsLoader.cpp
)
if(WIN32)
  list(APPEND Vulkan_layer_external_cpps ../../common/legacy/tools_windows.cpp)
endif()

add_library(Vulkan_layer SHARED
  ../interceptor/include/gitsPluginVulkan.h
  ../interceptor/include/vulkanExecWrap.h

  ${Vulkan_layer_external_cpps}
  ../interceptor/include/gitsPluginVulkan.cpp
  ../interceptor/vulkanPrePostAuto.cpp
  ${VulkanLayer_DEF}
)

source_group("External Files" FILES ${Vulkan_layer_external_cpps})
source_group("Generated Files" FILES ${VulkanLayer_DEF} REGULAR_EXPRESSION ".*Auto.*")

target_link_libraries(Vulkan_layer PRIVATE gits_recorder_interface common configuration)
if(WIN32)
  target_link_libraries(Vulkan_layer PRIVATE hud)
endif()

if(MSVC)
  set_target_properties(Vulkan_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
else()
  set_target_properties(Vulkan_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
  target_link_libraries(Vulkan_layer PRIVATE pthread dl)
endif()

set_target_properties(Vulkan_layer PROPERTIES FOLDER Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_layer> DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR} OPTIONAL)
endif()
install(TARGETS Vulkan_layer
  RUNTIME DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})
