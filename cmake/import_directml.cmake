# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DML_DIR)
  add_thirdparty_arg("--with-directml")
  set(DML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/DirectML")
endif()
