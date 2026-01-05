# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_plugin_utils STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(DirectX_plugin_utils
PRIVATE
  ${PLUGINS_DIR}/common/pluginUtils.h
  ${SRC_DIR}/gpuExecutionTracker.h
  ${SRC_DIR}/gpuExecutionTracker.cpp
  ${SRC_DIR}/taskScheduler.h
  ${SRC_DIR}/taskScheduler.cpp
)

target_include_directories(DirectX_plugin_utils
PUBLIC
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common
  ${SRC_DIR}
)

target_link_libraries(DirectX_plugin_utils 
PUBLIC
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_plugin_utils PROPERTIES FOLDER Plugins/DirectX/DirectX_common)
