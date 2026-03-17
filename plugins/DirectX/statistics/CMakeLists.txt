# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_statistics SHARED)
set_target_properties(DirectX_statistics PROPERTIES OUTPUT_NAME plugin)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(AUTO_GEN_SRC
  ${SRC_DIR}/statisticsLayerAuto.h
  ${SRC_DIR}/statisticsLayerAuto.cpp
)
set_auto_generated_property(FILE_LIST ${AUTO_GEN_SRC})

target_sources(DirectX_statistics
PRIVATE
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
  plugin.cpp
  statisticsLayerCustom.cpp
  statisticsService.h
  statisticsService.cpp
  config.yml
  ${AUTO_GEN_SRC}
)

target_link_libraries(DirectX_statistics PRIVATE
  DirectX_plugin_utils
)

set_target_properties(DirectX_statistics PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_statistics> DESTINATION Plugins/DirectX/statistics OPTIONAL)
endif()

install(TARGETS DirectX_statistics
  RUNTIME DESTINATION Plugins/DirectX/statistics
  LIBRARY DESTINATION Plugins/DirectX/statistics)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/statistics)