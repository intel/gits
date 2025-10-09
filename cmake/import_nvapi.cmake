# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED NVAPI_DIR)
  add_thirdparty_arg_setup("--with-nvapi" init_nvapi)
  set(NVAPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nvapi/")
endif()

function(init_nvapi)
  include_directories(${NVAPI_DIR})
endfunction()

