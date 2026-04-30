# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_hello_hud SHARED)
set_target_properties(DirectX_hello_hud PROPERTIES OUTPUT_NAME plugin)

target_sources(DirectX_hello_hud
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_link_libraries(DirectX_hello_hud PRIVATE
  DirectX_plugin_utils
  hud
)

set_target_properties(DirectX_hello_hud PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_hello_hud> DESTINATION Plugins/DirectX/hello_hud OPTIONAL)
endif()

install(TARGETS DirectX_hello_hud
  RUNTIME DESTINATION Plugins/DirectX/hello_hud
  LIBRARY DESTINATION Plugins/DirectX/hello_hud)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/hello_hud)