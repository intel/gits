# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

add_definitions(-DGITS_PLUGIN_DLL)
add_definitions(-DGITS_PLUGIN_EXPORT_API)

if(BUILD_DIRECTX_PLUGINS)
  add_subdirectory(DirectX/hello_plugin)
  add_subdirectory(DirectX/common/utils)
  add_subdirectory(DirectX/benchmark)
  add_subdirectory(DirectX/rtas_cache)
  add_subdirectory(DirectX/rtas_size_check)
  add_subdirectory(DirectX/adapter_spoof)
endif()
