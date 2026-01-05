# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_adapter_spoof SHARED)
set_target_properties(DirectX_adapter_spoof PROPERTIES OUTPUT_NAME plugin)

target_sources(DirectX_adapter_spoof
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_link_libraries(DirectX_adapter_spoof PRIVATE
  DirectX_plugin_utils
)

set_target_properties(DirectX_adapter_spoof PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_adapter_spoof> DESTINATION Plugins/DirectX/adapter_spoof OPTIONAL)
endif()

install(TARGETS DirectX_adapter_spoof
  RUNTIME DESTINATION Plugins/DirectX/adapter_spoof
  LIBRARY DESTINATION Plugins/DirectX/adapter_spoof)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/adapter_spoof)