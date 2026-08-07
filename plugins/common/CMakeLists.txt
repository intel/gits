# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(plugin_utils STATIC)

target_sources(plugin_utils
PRIVATE
  pluginUtils.h
  pluginUtils.cpp
)

target_include_directories(plugin_utils
PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PLUGINS_DIR}/common
)

target_link_libraries(plugin_utils
PUBLIC
  yamlcpp
  log
  common
)

set_target_properties(plugin_utils PROPERTIES FOLDER Plugins/common)
