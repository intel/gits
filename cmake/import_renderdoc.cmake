# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED RENDERDOC_ABS_PATH)
  install_dependencies("--with-renderdoc")
  set(RENDERDOC_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/renderdoc/renderdoc/api/app")
endif()

source_group("External Files" FILES
  ${RENDERDOC_ABS_PATH}/renderdoc_app.h
)

include_directories(${RENDERDOC_ABS_PATH})
