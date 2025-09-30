# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(Vulkan_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/Vulkan/codegen")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Vulkan_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "${Vulkan_SCRIPTS_DIR}/generate_vk.py"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/Vulkan_codegen.stamp"
  WORKING_DIRECTORY "${Vulkan_SCRIPTS_DIR}"
  DEPENDS
    "${Vulkan_SCRIPTS_DIR}/generate_vk.py"
    "${Vulkan_SCRIPTS_DIR}/generator_vulkan.py"
)
add_custom_target(Vulkan_codegen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Vulkan_codegen.stamp")
set_target_properties(Vulkan_codegen PROPERTIES FOLDER Vulkan)

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

add_dependencies(Vulkan_common Vulkan_codegen config_codegen)
add_dependencies(Vulkan_recorder Vulkan_codegen config_codegen)
add_dependencies(Vulkan_interceptor Vulkan_codegen config_codegen)
add_dependencies(Vulkan_layer Vulkan_codegen config_codegen)

set(Vulkan_Layer_files
  layer/VkLayer_vulkan_GITS_recorder.json
)

install(FILES ${Vulkan_Layer_files} DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})
