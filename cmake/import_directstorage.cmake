# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DSTORAGE_DIR)
  install_dependencies("--with-directstorage")
  set(DSTORAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/DirectStorage")
endif()
