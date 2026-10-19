# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED CEREAL_ABS_PATH)
  add_thirdparty_arg_setup("--with-cereal" init_cereal)
  set(CEREAL_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cereal")
endif()

function(init_cereal)
  include_directories("${CEREAL_ABS_PATH}/include")
endfunction()
