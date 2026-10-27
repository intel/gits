# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL_statistics SHARED)
set_target_properties(OpenGL_statistics PROPERTIES OUTPUT_NAME plugin PREFIX "")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(AUTO_GEN_SRC
  ${SRC_DIR}/statisticsLayerAuto.h
  ${SRC_DIR}/statisticsLayerAuto.cpp
)
set_auto_generated_property(FILE_LIST ${AUTO_GEN_SRC})

target_sources(OpenGL_statistics
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

target_include_directories(OpenGL_statistics
PRIVATE
  ${PLUGINS_DIR}
  ${SRC_DIR}
)

target_link_libraries(OpenGL_statistics PRIVATE
  OpenGL_layer_interface
  configuration
  yamlcpp
  plugin_utils
)

add_dependencies(OpenGL_statistics OpenGL_codegen)

set_target_properties(OpenGL_statistics PROPERTIES FOLDER Plugins/OpenGL)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL_statistics> DESTINATION Plugins/OpenGL/statistics OPTIONAL)
endif()

install(TARGETS OpenGL_statistics
  RUNTIME DESTINATION Plugins/OpenGL/statistics
  LIBRARY DESTINATION Plugins/OpenGL/statistics)

install(FILES config.yml
  DESTINATION Plugins/OpenGL/statistics)
