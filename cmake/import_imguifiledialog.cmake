# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED IMGUI_FILE_DIALOG_DIR)
  add_thirdparty_arg("--with-imguifiledialog")
  set(IMGUI_FILE_DIALOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuiFileDialog/")
endif()
