# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(DIRECTX_DIR ${CMAKE_SOURCE_DIR}/DirectX)
set(VULKAN_DIR ${CMAKE_SOURCE_DIR}/Vulkan)

function(set_auto_generated_property)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs FILE_LIST)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")
  foreach(FILE IN LISTS ARG_FILE_LIST)
    if(FILE MATCHES ".*Auto\\.h$" OR FILE MATCHES ".*Auto\\.cpp$")
      set_source_files_properties(${FILE} PROPERTIES GENERATED 1)
    endif()
  endforeach()
endfunction()

add_definitions(-DGITS_PLUGIN_DLL)
add_definitions(-DGITS_PLUGIN_EXPORT_API)

if(BUILD_DIRECTX_PLUGINS)
  set(D3D12_HEADERS_DIR ${AGILITY_SDK_DIR}/include)
  set(INTEL_EXT_DIR ${CMAKE_SOURCE_DIR}/third_party/IntelExtensions)

  add_subdirectory(DirectX/hello_plugin)
  add_subdirectory(DirectX/common/utils)
  add_subdirectory(DirectX/benchmark)
  add_subdirectory(DirectX/rtas_cache)
  add_subdirectory(DirectX/rtas_size_check)
  add_subdirectory(DirectX/adapter_spoof)
  add_subdirectory(DirectX/platform_portability)
  add_subdirectory(DirectX/statistics)
  add_subdirectory(DirectX/hello_hud)
  add_subdirectory(DirectX/cpu_patch)
endif()

if(BUILD_VULKAN_PLUGINS)
  add_subdirectory(Vulkan/benchmark)
  add_subdirectory(Vulkan/hello_plugin)
  add_subdirectory(Vulkan/statistics)
endif()
