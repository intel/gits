# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(VK_XML_PATH ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/registry/vk.xml)
set(VULKAN_DIR ${CMAKE_SOURCE_DIR}/Vulkan)

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

add_subdirectory(codegen)
add_subdirectory(common/layer_interface)
add_subdirectory(common/services)
add_subdirectory(common/utils)
add_subdirectory(common/coders)
add_subdirectory(subcapture)
add_subdirectory(recorder)
add_subdirectory(layer)
add_subdirectory(layers/trace)
add_subdirectory(layers/api_debug)
add_subdirectory(layers/resource_dumping)
add_subdirectory(layers/portability)
add_subdirectory(interceptor)
add_subdirectory(player)
