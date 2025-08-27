# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(D3D12_HEADERS_DIR ${AGILITY_SDK_DIR}/include)
set(DML_HEADERS_DIR ${DML_DIR}/include)
set(DSTORAGE_HEADERS_DIR ${DSTORAGE_DIR}/native/include)
set(DXC_HEADERS_DIR ${DXC_DIR}/include)
set(INTEL_EXT_DIR ${CMAKE_SOURCE_DIR}/third_party/IntelExtensions)
set(XESS_HEADERS_DIR ${XESS_SDK_DIR}/inc)

set(DIRECTX_DIR ${CMAKE_SOURCE_DIR}/DirectX)
set(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins)
set(BUILD_DIRECTX_PLUGINS ON)

function(set_auto_generated_property)
  # Parse function arguments
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs FILE_LIST)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")
  # Iterate over files
  foreach(FILE IN LISTS ARG_FILE_LIST)
    if(FILE MATCHES ".*Auto\\.h$" OR FILE MATCHES ".*Auto\\.cpp$")
      set_source_files_properties(${FILE} PROPERTIES GENERATED 1)
    endif()
  endforeach()
endfunction()

add_subdirectory(interceptors/d3d12_interceptor)
add_subdirectory(interceptors/dxgi_interceptor)
add_subdirectory(recorder)
add_subdirectory(codegen)
add_subdirectory(player)
add_subdirectory(common/layer_interface)
add_subdirectory(common/coders)
add_subdirectory(common/utils)
add_subdirectory(layers/trace)
add_subdirectory(layers/api_debug)
add_subdirectory(layers/subcapture)
add_subdirectory(layers/execution_serialization)
add_subdirectory(layers/resource_dumping)
add_subdirectory(layers/skip_calls)
add_subdirectory(layers/portability)
add_subdirectory(layers/dstorage_resources)
add_subdirectory(layers/imgui_hud)
add_subdirectory(layers/print_status)
add_subdirectory(layers/address_pinning)
add_subdirectory(${PLUGINS_DIR} DirectX_plugins)