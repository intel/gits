# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_cpu_patch SHARED)
set_target_properties(DirectX_cpu_patch PROPERTIES OUTPUT_NAME plugin)

set(PLUGINS_SRC
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
)
source_group("plugins" FILES ${PLUGINS_SRC})

set(UTILS_SRC
  resourceDump.h
  resourceDump.cpp
  descriptorHandleService.h
  descriptorHandleService.cpp
  shaderIdentifierService.h
  shaderIdentifierService.cpp
)
source_group("utils" FILES ${UTILS_SRC})

set(ROOT_SRC
  config.yml
  plugin.cpp
  layer.h
  layer.cpp
  instancesDump.h
  instancesDump.cpp
  bindingTableDump.h
  bindingTableDump.cpp
  executeIndirectDump.h
  executeIndirectDump.cpp
  patchService.h
  patchService.cpp
)
source_group("" FILES ${ROOT_SRC})

target_sources(DirectX_cpu_patch
PRIVATE
  ${PLUGINS_SRC}
  ${UTILS_SRC}
  ${ROOT_SRC}
)

target_link_libraries(DirectX_cpu_patch PRIVATE
  DirectX_plugin_utils
)

set_target_properties(DirectX_cpu_patch PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_cpu_patch> DESTINATION Plugins/DirectX/cpu_patch OPTIONAL)
endif()

install(TARGETS DirectX_cpu_patch
  RUNTIME DESTINATION Plugins/DirectX/cpu_patch
  LIBRARY DESTINATION Plugins/DirectX/cpu_patch)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/cpu_patch)
