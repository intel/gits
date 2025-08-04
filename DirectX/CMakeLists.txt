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
add_subdirectory(${PLUGINS_DIR} DirectX_plugins)

add_dependencies(DirectX_recorder
  DirectX_codegen
  DirectX_trace
  DirectX_dstorage_resources
  DirectX_layer_interface
  DirectX_coders
  DirectX_utils
)

add_dependencies(DirectX_trace
  DirectX_layer_interface
  DirectX_utils
)

add_dependencies(DirectX_api_debug
  DirectX_layer_interface
)

add_dependencies(DirectX_layer_interface
    DirectX_codegen
    config_codegen
)

add_dependencies(DirectX_player
  DirectX_layer_interface
  DirectX_utils
)

add_dependencies(DirectX_subcapture
  DirectX_layer_interface
  DirectX_dstorage_resources
  DirectX_coders
  DirectX_utils
)

add_dependencies(DirectX_execution_serialization
  DirectX_layer_interface
  DirectX_coders
  DirectX_utils
)

add_dependencies(DirectX_coders
  DirectX_layer_interface
)

add_dependencies(DirectX_dstorage_resources
  DirectX_layer_interface
)

add_dependencies(DirectX_resource_dumping
  DirectX_layer_interface
  DirectX_utils
)

add_dependencies(DirectX_skip_calls
  DirectX_layer_interface
)

add_dependencies(DirectX_portability
    DirectX_layer_interface
    DirectX_utils
)

add_dependencies(DirectX_imgui_hud
  DirectX_layer_interface
)

add_dependencies(DirectX_utils
    config_codegen
)
target_include_directories(DirectX_subcapture PRIVATE ${ARGS_SOURCE_DIR})
target_include_directories(DirectX_recorder PRIVATE ${ARGS_SOURCE_DIR})
target_include_directories(DirectX_player PRIVATE ${ARGS_SOURCE_DIR})

target_link_libraries(DirectX_subcapture PRIVATE
    DirectX_layer_interface
    DirectX_dstorage_resources
    DirectX_utils
    DirectX_coders
    common
    configuration
)

target_link_libraries(DirectX_execution_serialization PRIVATE
  DirectX_utils
  DirectX_coders
  common
  configuration
)

target_link_libraries(DirectX_recorder PRIVATE
  DirectX_api_debug
  DirectX_trace
  DirectX_dstorage_resources
  DirectX_coders
  DirectX_imgui_hud
  DirectX_utils
  DirectX_resource_dumping
  DirectX_portability
  configuration
  DirectX_layer_interface
  common
)

target_link_libraries(DirectX_player PRIVATE
  DirectX_layer_interface
  DirectX_api_debug
  DirectX_trace
  DirectX_resource_dumping
  DirectX_subcapture
  DirectX_execution_serialization
  DirectX_coders
  DirectX_imgui_hud
  DirectX_utils
  DirectX_skip_calls
  DirectX_portability
  configuration
  common
)

target_link_libraries(DirectX_utils PRIVATE common configuration)

target_link_libraries(DirectX_resource_dumping PRIVATE common configuration)

target_link_libraries(DirectX_api_debug PRIVATE common configuration)

target_link_libraries(DirectX_skip_calls PRIVATE common configuration)

target_link_libraries(DirectX_dstorage_resources PRIVATE common configuration)

target_link_libraries(DirectX_trace PRIVATE common configuration)

target_link_libraries(DirectX_coders PRIVATE common configuration)

target_link_libraries(DirectX_portability PRIVATE common configuration)
