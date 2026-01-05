# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED FASTIO_DIR)
  add_thirdparty_arg_setup("--with-fastio" init_fastio)
  set(FASTIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fast_io/")
endif()

function(init_fastio)
  include_directories(${FASTIO_DIR})
endfunction()
