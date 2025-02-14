# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_benchmark SHARED)
set_target_properties(DirectX_benchmark PROPERTIES OUTPUT_NAME plugin)
target_link_libraries(DirectX_benchmark common)

set(SERVICES_DIR services)
set (SERVICES_SRC
  ${SERVICES_DIR}/cpuFrameBenchmarkService.h
  ${SERVICES_DIR}/cpuFrameBenchmarkService.cpp

)
source_group("services" FILES ${SERVICES_SRC})

set(UTILS_DIR utils)
set (UTILS_SRC
  ${UTILS_DIR}/benchmarkUtils.h
  ${UTILS_DIR}/benchmarkUtils.cpp
  ${UTILS_DIR}/TaskScheduler.h
  ${UTILS_DIR}/TaskScheduler.cpp
)
source_group("utils" FILES ${UTILS_SRC})

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
  ${UTILS_SRC}
)

target_include_directories(DirectX_benchmark
PRIVATE
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${AGILITY_SDK_DIR}/include
  ${CMAKE_SOURCE_DIR}/DirectX/common/layer_interface
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common
  ${SERVICES_DIR}
  ${UTILS_DIR}
)

add_dependencies(DirectX_benchmark
    DirectX_layer_interface
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
