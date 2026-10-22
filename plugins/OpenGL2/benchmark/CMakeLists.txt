# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL2_benchmark SHARED)
set_target_properties(OpenGL2_benchmark PROPERTIES OUTPUT_NAME plugin)

target_sources(OpenGL2_benchmark
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.h
  config.yml
  cpuFrameBenchmarkService.h
  cpuFrameBenchmarkService.cpp
)

target_include_directories(OpenGL2_benchmark
PRIVATE
  ${PLUGINS_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenGL2_benchmark PRIVATE
  OpenGL2_layer_interface
  configuration
  yamlcpp
  plugin_utils
)

add_dependencies(OpenGL2_benchmark OpenGL2_codegen)

set_target_properties(OpenGL2_benchmark PROPERTIES FOLDER Plugins/OpenGL2)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL2_benchmark> DESTINATION Plugins/OpenGL2/benchmark OPTIONAL)
endif()

install(TARGETS OpenGL2_benchmark
  RUNTIME DESTINATION Plugins/OpenGL2/benchmark
  LIBRARY DESTINATION Plugins/OpenGL2/benchmark)

install(FILES config.yml
  DESTINATION Plugins/OpenGL2/benchmark)
