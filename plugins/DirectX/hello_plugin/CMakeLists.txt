# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_hello_plugin SHARED)
set_target_properties(DirectX_hello_plugin PROPERTIES OUTPUT_NAME plugin)
target_link_libraries(DirectX_hello_plugin PRIVATE common)

target_sources(DirectX_hello_plugin
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_include_directories(DirectX_hello_plugin
PRIVATE
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${AGILITY_SDK_DIR}/include
  ${CMAKE_SOURCE_DIR}/DirectX/common/layer_interface
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common/
)

add_dependencies(DirectX_hello_plugin
    DirectX_layer_interface
    common
)

set_target_properties(DirectX_hello_plugin PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_hello_plugin> DESTINATION Plugins/DirectX/hello_plugin OPTIONAL)
endif()

install(TARGETS DirectX_hello_plugin
  RUNTIME DESTINATION Plugins/DirectX/hello_plugin
  LIBRARY DESTINATION Plugins/DirectX/hello_plugin)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/hello_plugin)