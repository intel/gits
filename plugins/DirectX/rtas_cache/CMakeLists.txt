# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_rtas_cache SHARED)
set_target_properties(DirectX_rtas_cache PROPERTIES OUTPUT_NAME plugin)
target_link_libraries(DirectX_rtas_cache common DirectX_utils DirectX_plugin_utils)

set(PLUGINS_SRC
  ${PLUGINS_DIR}/IPlugin.h
  ${PLUGINS_DIR}/common/pluginUtils.h
)
source_group("plugins" FILES ${PLUGINS_SRC})

set(UTILS_SRC
  resourceDump.h
  resourceDump.cpp
)
source_group("utils" FILES ${UTILS_SRC})

set(ROOT_SRC
  config.yml
  plugin.cpp
  layer.h
  layer.cpp
  rtasSerializer.h
  rtasSerializer.cpp
  rtasDeserializer.h
  rtasDeserializer.cpp
)
source_group("" FILES ${ROOT_SRC})

target_sources(DirectX_rtas_cache
PRIVATE
  ${PLUGINS_SRC}
  ${UTILS_SRC}
  ${ROOT_SRC}
)

target_include_directories(DirectX_rtas_cache
PRIVATE
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${AGILITY_SDK_DIR}/include
  ${CMAKE_SOURCE_DIR}/DirectX/common/layer_interface
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common/
  ${PLUGINS_DIR}/DirectX/common/utils
)

set_target_properties(DirectX_rtas_cache PROPERTIES FOLDER Plugins/DirectX)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:DirectX_rtas_cache> DESTINATION Plugins/DirectX/rtas_cache OPTIONAL)
endif()

install(TARGETS DirectX_rtas_cache
  RUNTIME DESTINATION Plugins/DirectX/rtas_cache
  LIBRARY DESTINATION Plugins/DirectX/rtas_cache)

install(FILES config.yml 
  DESTINATION Plugins/DirectX/rtas_cache)