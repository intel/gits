# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_platform_portability SHARED)
set_target_properties(DirectX_platform_portability PROPERTIES OUTPUT_NAME plugin)

target_sources(DirectX_platform_portability
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  config.yml
)

target_link_libraries(DirectX_platform_portability PRIVATE
  DirectX_plugin_utils
)

set_target_properties(DirectX_platform_portability PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_platform_portability> DESTINATION Plugins/DirectX/platform_portability OPTIONAL)
endif()

install(TARGETS DirectX_platform_portability
  RUNTIME DESTINATION Plugins/DirectX/platform_portability
  LIBRARY DESTINATION Plugins/DirectX/platform_portability)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/platform_portability)
