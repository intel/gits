# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_benchmark SHARED)
set_target_properties(Vulkan_benchmark PROPERTIES OUTPUT_NAME plugin)

target_compile_definitions(Vulkan_benchmark
PRIVATE
  GITS_PLUGIN_DLL
  GITS_PLUGIN_EXPORT_API
)

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)

set(SERVICES_DIR services)
set(SERVICES_SRC
  ${SERVICES_DIR}/cpuFrameBenchmarkService.h
  ${SERVICES_DIR}/cpuFrameBenchmarkService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

target_sources(Vulkan_benchmark
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  plugin.cpp
  layer.h
  layer.cpp
  config.h
  config.yml
  ${SERVICES_SRC}
)

target_include_directories(Vulkan_benchmark
PRIVATE
  ${PLUGINS_DIR}
)

# The heavy GITS core libraries (common, configuration, log) arrive transitively
# through Vulkan_layer_interface, so a single thin link target plus yaml-cpp for
# config parsing is enough.  This mirrors the Vulkan statistics plugin.
target_link_libraries(Vulkan_benchmark
PRIVATE
  Vulkan_layer_interface
  yamlcpp
)

set_target_properties(Vulkan_benchmark PROPERTIES FOLDER Plugins/Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_benchmark> DESTINATION Plugins/Vulkan/benchmark OPTIONAL)
endif()

install(TARGETS Vulkan_benchmark
  RUNTIME DESTINATION Plugins/Vulkan/benchmark
  LIBRARY DESTINATION Plugins/Vulkan/benchmark)

install(FILES config.yml
  DESTINATION Plugins/Vulkan/benchmark)
