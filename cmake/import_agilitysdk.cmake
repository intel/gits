# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED AGILITY_SDK_DIR)
  install_dependencies("--with-agility-sdk")
  set(AGILITY_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/AgilitySDK/")
endif()
