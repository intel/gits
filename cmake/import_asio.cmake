# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED ASIO_ABS_PATH)
  add_thirdparty_arg_setup("--with-asio" init_asio)
  set(ASIO_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio")
endif()

function(init_asio)
  # Header only, and it selects its standalone (non-Boost) mode automatically for
  # MSVC with C++11 or later. Asio additionally needs _WIN32_WINNT pinned on
  # Windows; that is applied per consuming target rather than here, because
  # forcing an API level across the whole build would affect unrelated code.
  include_directories("${ASIO_ABS_PATH}/include")
endfunction()
