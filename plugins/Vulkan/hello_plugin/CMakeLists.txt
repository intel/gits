# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_hello_plugin SHARED)
set_target_properties(Vulkan_hello_plugin PROPERTIES OUTPUT_NAME plugin)

target_compile_definitions(Vulkan_hello_plugin
PRIVATE
  GITS_PLUGIN_DLL
  GITS_PLUGIN_EXPORT_API
)

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)

target_sources(Vulkan_hello_plugin
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_include_directories(Vulkan_hello_plugin
PRIVATE
  ${PLUGINS_DIR}
)

target_link_libraries(Vulkan_hello_plugin
PRIVATE
  Vulkan_layer_interface
  yamlcpp
  configuration
  common
  log
)

set_target_properties(Vulkan_hello_plugin PROPERTIES FOLDER Plugins/Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_hello_plugin> DESTINATION Plugins/Vulkan/hello_plugin OPTIONAL)
endif()

install(TARGETS Vulkan_hello_plugin
  RUNTIME DESTINATION Plugins/Vulkan/hello_plugin
  LIBRARY DESTINATION Plugins/Vulkan/hello_plugin)

install(FILES config.yml
  DESTINATION Plugins/Vulkan/hello_plugin)
