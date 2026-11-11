# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Example for module authors. The build-tree Studio runs with it, but it is never installed.
add_library(hello_module SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/helloPanel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/helloPanel.cpp
)

target_link_libraries(hello_module
PRIVATE
  module_common
  gui_common
  common_interface
  imgui_common
  log
  configuration
  stream
)

add_dependencies(hello_module config_codegen)

set_target_properties(hello_module PROPERTIES FOLDER "${GUI_MODULES_FOLDER}")

gui_module(hello_module hello)
