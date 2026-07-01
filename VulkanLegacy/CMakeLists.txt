# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(VulkanLegacy_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/VulkanLegacy/codegen")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/VulkanLegacy_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "${VulkanLegacy_SCRIPTS_DIR}/generate_vk.py"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/VulkanLegacy_codegen.stamp"
  WORKING_DIRECTORY "${VulkanLegacy_SCRIPTS_DIR}"
  DEPENDS
    "${VulkanLegacy_SCRIPTS_DIR}/generate_vk.py"
    "${VulkanLegacy_SCRIPTS_DIR}/generator_vulkan.py"
)
add_custom_target(VulkanLegacy_codegen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/VulkanLegacy_codegen.stamp")
set_target_properties(VulkanLegacy_codegen PROPERTIES FOLDER VulkanLegacy)

# GENERATED property is set in CMakeLists.txt in each directory, but this
# staying here doesn't do any harm
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanIDs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptor/vulkanPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanLogAuto.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/common/vulkanLogAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanTracerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/vulkanTracerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanHeader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanDriversAuto.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptor/vkPlugin.def
  ${CMAKE_CURRENT_SOURCE_DIR}/layer/vkLayer.def
  ${CMAKE_CURRENT_SOURCE_DIR}/common/vulkanArgumentsAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanArgumentsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/vulkanStructStorageAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanStructStorageAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/vulkanFunctions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanIDswitch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/include/vulkanRecorderWrapperAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/vulkanRecorderWrapperAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recorder/include/vulkanRecorderWrapperIfaceAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vulkanLuaEnums.h
  PROPERTIES GENERATED 1
)

add_subdirectory(common)
add_subdirectory(recorder)
add_subdirectory(interceptor)
add_subdirectory(layer)

add_dependencies(VulkanLegacy_common VulkanLegacy_codegen config_codegen)
add_dependencies(VulkanLegacy_recorder VulkanLegacy_codegen config_codegen)
add_dependencies(VulkanLegacy_interceptor VulkanLegacy_codegen config_codegen)
add_dependencies(VulkanLegacy_layer VulkanLegacy_codegen config_codegen)

set(VulkanLegacy_Layer_files
  layer/VkLayer_vulkan_GITS_recorder_legacy.json
)

install(FILES ${VulkanLegacy_Layer_files} DESTINATION Recorder/${VulkanLayerLegacy_INSTALL_SUBDIR})
