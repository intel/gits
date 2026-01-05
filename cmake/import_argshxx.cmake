# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED ARGS_SOURCE_DIR)
  add_thirdparty_arg("--with-argshxx")
  set(ARGS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/argshxx")
endif()
