# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================


set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanIDs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanLogAuto.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanLogAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanTracerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanTracerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanHeader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanDriversAuto.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanArgumentsAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanArgumentsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanStructStorageAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanStructStorageAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanFunctions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanIDswitch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanLuaEnums.h
  PROPERTIES GENERATED 1
)

add_library(Vulkan_common
  include/vkFunction.h
  include/vkWindowing.h
  include/vk_platform.h
  include/vk_layer.h
  include/vulkanArgumentsAuto.h
  include/vulkanArgumentsBasic.h
  include/vulkanDrivers.h
  include/vulkanDriversAuto.inl
  include/vulkanFunctions.h
  include/vulkanHeader.h
  include/vulkanIDs.h
  include/vulkanIDswitch.h
  include/vulkanLibrary.h
  include/vulkanLog.h
  include/vulkanLogAuto.inl
  include/vulkanLuaEnums.h
  include/vulkanPlayerRunWrap.h
  include/vulkanPreToken.h
  include/vulkanStateDynamic.h
  include/vulkanStateTracking.h
  include/vulkanStructStorageAuto.h
  include/vulkanStructStorageBasic.h
  include/vulkanTools.h
  include/vulkanTools_lite.h
  include/vulkanTracerAuto.h
  include/vulkanTypes.h
  include/vulkan_apis_iface.h
  include/vulkan_basic.h
  include/vulkanCCodeWriteWrap.h
  include/vulkanPNextWrappers.inl
  include/vulkanInternalShaderModules.h

  vkFunction.cpp
  vkWindowing.cpp
  vulkan_apis_iface.cpp
  vulkanArgumentsBasic.cpp
  vulkanArgumentsWrap.cpp
  vulkanArgumentsAuto.cpp
  vulkanDrivers.cpp
  vulkanFunctions.cpp
  vulkanStructStorageAuto.cpp
  vulkanLibrary.cpp
  vulkanLog.cpp
  vulkanLogAuto.cpp
  vulkanPreToken.cpp
  vulkanStateDynamic.cpp
  vulkanStructStorageBasic.cpp
  vulkanStructStorageWrap.cpp
  vulkanTools.cpp
  vulkanTools_lite.cpp
  vulkanTracerAuto.cpp
  vulkanCCodeWriteWrap.cpp
  vulkanInternalShaderModules.cpp
)

source_group("Generated Files" REGULAR_EXPRESSION ".*Auto.*")

target_link_libraries(Vulkan_common common)

set_target_properties(Vulkan_common PROPERTIES FOLDER Vulkan)
