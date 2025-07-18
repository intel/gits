# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_rtas_size_check SHARED)
set_target_properties(DirectX_rtas_size_check PROPERTIES OUTPUT_NAME plugin)
target_link_libraries(DirectX_rtas_size_check common)

target_sources(DirectX_rtas_size_check
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_include_directories(DirectX_rtas_size_check
PRIVATE
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${AGILITY_SDK_DIR}/include
  ${CMAKE_SOURCE_DIR}/DirectX/common/layer_interface
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common/
)

add_dependencies(DirectX_rtas_size_check
    DirectX_layer_interface
)

set_target_properties(DirectX_rtas_size_check PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_rtas_size_check> DESTINATION Plugins/DirectX/rtas_size_check OPTIONAL)
endif()

install(TARGETS DirectX_rtas_size_check
  RUNTIME DESTINATION Plugins/DirectX/rtas_size_check
  LIBRARY DESTINATION Plugins/DirectX/rtas_size_check)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/rtas_size_check)
