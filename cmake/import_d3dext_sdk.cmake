# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED INTEL_EXT_DIR)
  add_thirdparty_arg("--with-d3dext-sdk")
  set(INTEL_EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3dext-sdk/sdk")
endif()
