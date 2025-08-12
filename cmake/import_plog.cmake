# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED PLOG_SOURCE_DIR)
  install_dependencies("--with-plog")
  set(PLOG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/plog")
endif()
