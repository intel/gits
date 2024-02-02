# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED JSON_ABS_PATH)
  install_dependencies("--with-json")
  set(JSON_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json")
endif()

include_directories("${JSON_ABS_PATH}/single_include")
