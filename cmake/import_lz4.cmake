# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED LZ4_ABS_PATH)
  add_thirdparty_arg_setup("--with-lz4" init_lz4)
  set(LZ4_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lz4/lib")
endif()

function(init_lz4)
  add_library(lz4
    ${LZ4_ABS_PATH}/lz4.c
    ${LZ4_ABS_PATH}/lz4.h
  )

  set_target_properties(lz4 PROPERTIES FOLDER External)

  include_directories(${LZ4_ABS_PATH})
endfunction()
