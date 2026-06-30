# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(VK_XML_PATH ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/registry/vk.xml)
set(VULKAN2_DIR ${CMAKE_SOURCE_DIR}/Vulkan2)

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
add_subdirectory(common/coders)
add_subdirectory(layer)
add_subdirectory(recorder)
add_subdirectory(player)
