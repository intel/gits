# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DXC_DIR)
  install_dependencies("--with-dxc")
  set(DXC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc")
endif()
