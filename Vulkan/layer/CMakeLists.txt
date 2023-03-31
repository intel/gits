# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
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

add_library(Vulkan_layer SHARED
  ../interceptor/include/gitsPluginVulkan.h
  ../interceptor/include/vulkanExecWrap.h

  ../../common/exception.cpp
  ../../common/dynamic_linker.cpp
  ../../recorder/gitsLoader.cpp
  ../interceptor/include/gitsPluginVulkan.cpp
  ../interceptor/vulkanPrePostAuto.cpp
  ../../common/log.cpp
  ${VulkanLayer_DEF}
)

source_group("External Files" FILES
  ../../common/exception.cpp
  ../../common/dynamic_linker.cpp
  ../../recorder/gitsLoader.cpp
  ../../common/log.cpp
)
source_group("Generated Files" FILES ${VulkanLayer_DEF} REGULAR_EXPRESSION ".*Auto.*")

if(MSVC)
  set_target_properties(Vulkan_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
else()
  set_target_properties(Vulkan_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
  target_link_libraries(Vulkan_layer pthread dl)
endif()
target_link_libraries(Vulkan_layer Boost::thread Boost::filesystem)

set_target_properties(Vulkan_layer PROPERTIES FOLDER Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_layer> DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR} OPTIONAL)
endif()
install(TARGETS Vulkan_layer
  RUNTIME DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})