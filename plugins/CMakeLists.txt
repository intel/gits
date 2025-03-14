# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/common)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

add_definitions(-DGITS_PLUGIN_DLL)
add_definitions(-DGITS_PLUGIN_EXPORT_API)

if(BUILD_DIRECTX_PLUGINS)
  add_subdirectory(DirectX/hello_plugin)
  add_subdirectory(DirectX/benchmark)
  add_subdirectory(DirectX/rtas_cache)
endif()
