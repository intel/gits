# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(rtsr_plugin_common STATIC
  rtsrPluginConfig.cpp
  rtsrPluginConfig.h
)

target_include_directories(rtsr_plugin_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rtsr_plugin_common
PUBLIC
  rtsr_module_shared
PRIVATE
  yamlcpp
)

set_target_properties(rtsr_plugin_common PROPERTIES
  FOLDER "${GUI_MODULES_FOLDER}/rtsr"
  POSITION_INDEPENDENT_CODE ON
)
