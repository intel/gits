# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED FASTIO_DIR)
  install_dependencies("--with-fastio")
  set(FASTIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fast_io/")
  include_directories(${FASTIO_DIR})
endif()
