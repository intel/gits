# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED XESS_SDK_DIR)
  add_thirdparty_arg("--with-xess")
  set(XESS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xess")
endif()
