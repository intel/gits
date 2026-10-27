# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL_benchmark SHARED)
set_target_properties(OpenGL_benchmark PROPERTIES OUTPUT_NAME plugin PREFIX "")

target_sources(OpenGL_benchmark
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

target_include_directories(OpenGL_benchmark
PRIVATE
  ${PLUGINS_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenGL_benchmark PRIVATE
  OpenGL_layer_interface
  configuration
  yamlcpp
  plugin_utils
)

add_dependencies(OpenGL_benchmark OpenGL_codegen)

set_target_properties(OpenGL_benchmark PROPERTIES FOLDER Plugins/OpenGL)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL_benchmark> DESTINATION Plugins/OpenGL/benchmark OPTIONAL)
endif()

install(TARGETS OpenGL_benchmark
  RUNTIME DESTINATION Plugins/OpenGL/benchmark
  LIBRARY DESTINATION Plugins/OpenGL/benchmark)

install(FILES config.yml
  DESTINATION Plugins/OpenGL/benchmark)
