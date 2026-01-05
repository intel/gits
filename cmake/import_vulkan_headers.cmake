# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED VULKAN_HEADERS_DIR)
  add_thirdparty_arg("--with-vulkan-headers")
  set(VULKAN_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include/")
endif()