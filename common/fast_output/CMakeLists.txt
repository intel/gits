# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(FASTOUTPUT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(FASTOUTPUT_HEADER_DIR ${FASTOUTPUT_SOURCE_DIR}/include)

set(FASTOUTPUT_CPP_FILES
  ${FASTOUTPUT_SOURCE_DIR}/fastOStream.cpp
)

set(FASTOUTPUT_HEADER_FILES
  ${FASTOUTPUT_HEADER_DIR}/fastOStream.h
)

add_library(fast_output STATIC ${FASTOUTPUT_CPP_FILES} ${FASTOUTPUT_HEADER_FILES})

target_include_directories(
  fast_output PUBLIC
  ${FASTIO_DIR}/include
  ${COMMON_HEADER_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${FASTOUTPUT_SOURCE_DIR}/include
)

target_link_libraries(fast_output PRIVATE
  common
  log
)

set_target_properties(fast_output PROPERTIES FOLDER "common/trace")

add_subdirectory(ipc)

