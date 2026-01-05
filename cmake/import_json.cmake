# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED JSON_ABS_PATH)
  add_thirdparty_arg_setup("--with-json" init_json)
  set(JSON_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json")
endif()

function(init_json)
  include_directories("${JSON_ABS_PATH}/single_include")
endfunction()
