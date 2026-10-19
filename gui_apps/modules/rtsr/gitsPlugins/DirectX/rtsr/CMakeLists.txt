# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_rtsr SHARED)
set_target_properties(DirectX_rtsr PROPERTIES OUTPUT_NAME plugin)

# plugins/CMakeLists.txt applies these with a directory scoped add_definitions(),
# which does not reach targets declared under gui_apps/.
target_compile_definitions(DirectX_rtsr
PRIVATE
  GITS_PLUGIN_DLL
  GITS_PLUGIN_EXPORT_API
)

target_sources(DirectX_rtsr
PRIVATE
  ${CMAKE_SOURCE_DIR}/plugins/IPlugin.h
  ${CMAKE_SOURCE_DIR}/plugins/common/pluginUtils.h
  plugin.cpp
  layer.h
  layer.cpp
  pipelineTracker.h
  pipelineTracker.cpp
  performanceTracker.h
  performanceTracker.cpp
  shaderInfo.h
  shaderInfo.cpp
  utils/gpuTimer.h
  utils/gpuTimer.cpp
  config.yml
)

target_link_libraries(DirectX_rtsr PRIVATE
  DirectX_plugin_utils
  rtsr_plugin_common
  rtsr_module_shared
  d3d12.lib
)

set_target_properties(DirectX_rtsr PROPERTIES FOLDER "${GUI_MODULES_FOLDER}/rtsr")

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_rtsr> DESTINATION Plugins/DirectX/rtsr OPTIONAL)
endif()

install(TARGETS DirectX_rtsr
  RUNTIME DESTINATION Plugins/DirectX/rtsr
  LIBRARY DESTINATION Plugins/DirectX/rtsr)

install(FILES config.yml
  DESTINATION Plugins/DirectX/rtsr)
