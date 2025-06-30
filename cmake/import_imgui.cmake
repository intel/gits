# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED IMGUI_SOURCE_DIR)
  install_dependencies("--with-imgui")
  set(IMGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
  set(IMGUI_BACKEND_DIR "${IMGUI_SOURCE_DIR}/backends")
endif()
