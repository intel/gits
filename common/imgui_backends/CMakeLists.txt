# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(WIN32 AND WITH_VULKAN)
  add_subdirectory(imgui_win32_vulkan)
endif()
