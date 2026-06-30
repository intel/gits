# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_statistics SHARED)
set_target_properties(Vulkan_statistics PROPERTIES OUTPUT_NAME plugin)

target_compile_definitions(Vulkan_statistics
PRIVATE
  GITS_PLUGIN_DLL
  GITS_PLUGIN_EXPORT_API
)

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(AUTO_GEN_SRC
  ${SRC_DIR}/statisticsLayerAuto.h
  ${SRC_DIR}/statisticsLayerAuto.cpp
)
set_auto_generated_property(FILE_LIST ${AUTO_GEN_SRC})

target_sources(Vulkan_statistics
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  statisticsLayerCustom.cpp
  statisticsService.h
  statisticsService.cpp
  config.yml
  ${AUTO_GEN_SRC}
)

target_include_directories(Vulkan_statistics
PRIVATE
  ${PLUGINS_DIR}
)

target_link_libraries(Vulkan_statistics PRIVATE
  Vulkan_layer_interface
  yamlcpp
  configuration
  common
  log
)

set_target_properties(Vulkan_statistics PROPERTIES FOLDER Plugins/Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_statistics> DESTINATION Plugins/Vulkan/statistics OPTIONAL)
endif()

install(TARGETS Vulkan_statistics
  RUNTIME DESTINATION Plugins/Vulkan/statistics
  LIBRARY DESTINATION Plugins/Vulkan/statistics)

install(FILES config.yml
  DESTINATION Plugins/Vulkan/statistics)
