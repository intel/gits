# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================
set(GUI_MODULE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GUI_MODULE_COMMON_HEADER_DIR ${GUI_MODULE_COMMON_DIR}/include)

set(MODULE_COMMON_SOURCE
  ${GUI_MODULE_COMMON_DIR}/moduleRegistry.cpp
  ${GUI_MODULE_COMMON_HEADER_DIR}/moduleBasePanel.h
  ${GUI_MODULE_COMMON_HEADER_DIR}/moduleRegistry.h
)

add_library(module_common STATIC ${MODULE_COMMON_SOURCE})
# source_group("ImGui-AddOns" FILES ${IMGUI_ADDONS_SOURCE})
# source_group(TREE ${GUI_MODULES_DIR} FILES ${MODULE_COMMON_SOURCE})
target_include_directories(module_common PUBLIC ${GUI_MODULE_COMMON_HEADER_DIR})

target_link_libraries(module_common
  PUBLIC gui_common common_interface imgui_common
  PRIVATE log configuration)
add_dependencies(module_common config_codegen)
set_target_properties(module_common PROPERTIES FOLDER "${GUI_MODULES_FOLDER}")
