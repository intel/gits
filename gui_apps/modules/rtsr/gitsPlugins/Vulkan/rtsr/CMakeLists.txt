# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_rtsr SHARED)
set_target_properties(Vulkan_rtsr PROPERTIES OUTPUT_NAME plugin)

target_compile_definitions(Vulkan_rtsr PRIVATE
  GITS_PLUGIN_DLL
  GITS_PLUGIN_EXPORT_API
)

target_sources(Vulkan_rtsr PRIVATE
  ${CMAKE_SOURCE_DIR}/plugins/IPlugin.h
  plugin.cpp
  layer.cpp
  layer.h
  performanceTracker.cpp
  performanceTracker.h
  pipelineStateSnapshot.cpp
  pipelineStateSnapshot.h
  pipelineTracker.cpp
  pipelineTracker.h
  shaderModuleTracker.cpp
  shaderModuleTracker.h
  vulkanStateDocument.cpp
  vulkanStateDocument.h
  vulkanContext.cpp
  vulkanContext.h
  config.yml
)

target_include_directories(Vulkan_rtsr PRIVATE
  ${CMAKE_SOURCE_DIR}/plugins
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_rtsr PRIVATE
  Vulkan_layer_interface
  Vulkan_coders
  rtsr_plugin_common
  rtsr_module_shared
  configuration
  common
  log
)

set_target_properties(Vulkan_rtsr PROPERTIES
  FOLDER "${GUI_MODULES_FOLDER}/rtsr"
)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_rtsr>
    DESTINATION Plugins/Vulkan/rtsr
    OPTIONAL
  )
endif()

install(TARGETS Vulkan_rtsr
  RUNTIME DESTINATION Plugins/Vulkan/rtsr
  LIBRARY DESTINATION Plugins/Vulkan/rtsr
)
install(FILES config.yml DESTINATION Plugins/Vulkan/rtsr)
