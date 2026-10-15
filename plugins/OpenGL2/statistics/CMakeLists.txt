# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL2_statistics SHARED)
set_target_properties(OpenGL2_statistics PROPERTIES OUTPUT_NAME plugin)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(AUTO_GEN_SRC
  ${SRC_DIR}/statisticsLayerAuto.h
  ${SRC_DIR}/statisticsLayerAuto.cpp
)
set_auto_generated_property(FILE_LIST ${AUTO_GEN_SRC})

target_sources(OpenGL2_statistics
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

target_include_directories(OpenGL2_statistics
PRIVATE
  ${PLUGINS_DIR}
  ${SRC_DIR}
)

target_link_libraries(OpenGL2_statistics PRIVATE
  OpenGL2_layer_interface
  configuration
  yamlcpp
  plugin_utils
)

add_dependencies(OpenGL2_statistics OpenGL2_codegen)

set_target_properties(OpenGL2_statistics PROPERTIES FOLDER Plugins/OpenGL2)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL2_statistics> DESTINATION Plugins/OpenGL2/statistics OPTIONAL)
endif()

install(TARGETS OpenGL2_statistics
  RUNTIME DESTINATION Plugins/OpenGL2/statistics
  LIBRARY DESTINATION Plugins/OpenGL2/statistics)

install(FILES config.yml
  DESTINATION Plugins/OpenGL2/statistics)
