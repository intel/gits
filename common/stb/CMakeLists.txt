# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(STB_IMPL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(STREAM_SRC_SOURCES
  ${STB_IMPL_SOURCE_DIR}/stbImage.cpp
)

add_library(stb_implementation STATIC ${STREAM_SRC_SOURCES})

target_include_directories(stb_implementation
PRIVATE
  ${STB_IMPL_SOURCE_DIR}
  ${STB_DIR}
)

set_target_properties(stb_implementation PROPERTIES FOLDER "common")
