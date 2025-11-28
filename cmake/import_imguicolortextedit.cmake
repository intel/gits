# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED IMGUI_COLOR_TEXT_EDIT_DIR)
  add_thirdparty_arg("--with-imguicolortextedit")
  set(IMGUI_COLOR_TEXT_EDIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuiColorTextEdit/")
endif()
