# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_plugin_utils STATIC)
target_link_libraries(DirectX_plugin_utils common)

target_sources(DirectX_plugin_utils
PRIVATE
  ${PLUGINS_DIR}/common/pluginUtils.h
  gpuExecutionTracker.h
  gpuExecutionTracker.cpp
  taskScheduler.h
  taskScheduler.cpp
  benchmarkUtils.h
  benchmarkUtils.cpp
)

target_include_directories(DirectX_plugin_utils
PRIVATE
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${AGILITY_SDK_DIR}/include
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${PLUGINS_DIR}/common
)

set_target_properties(DirectX_plugin_utils PROPERTIES FOLDER Plugins/DirectX/DirectX_common)
