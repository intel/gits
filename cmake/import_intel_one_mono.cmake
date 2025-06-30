# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED INTELONEMONO_SOURCE_DIR)
  install_dependencies("--with-intel-one-mono")
  set(INTELONEMONO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/intel-one-mono")
  set(INTELONEMONO_FONT_REGULAR "${INTELONEMONO_SOURCE_DIR}/fonts/ttf/IntelOneMono-Regular.ttf")
endif()
