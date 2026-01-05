# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_benchmark SHARED)
set_target_properties(DirectX_benchmark PROPERTIES OUTPUT_NAME plugin)

set(SERVICES_DIR services)
set (SERVICES_SRC
  ${SERVICES_DIR}/cpuFrameBenchmarkService.h
  ${SERVICES_DIR}/cpuFrameBenchmarkService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

target_sources(DirectX_benchmark
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
  config.h
  ${SERVICES_SRC}
)

target_link_libraries(DirectX_benchmark PRIVATE
  DirectX_plugin_utils
)

set_target_properties(DirectX_benchmark PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_benchmark> DESTINATION Plugins/DirectX/benchmark OPTIONAL)
endif()

install(TARGETS DirectX_benchmark
  RUNTIME DESTINATION Plugins/DirectX/benchmark
  LIBRARY DESTINATION Plugins/DirectX/benchmark)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/benchmark)
