# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FASTOUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "FastOutput directory")

set(FASTOUTPUT_SRC
  ${SRC_DIR}/fastOStream.h
  ${SRC_DIR}/fastOStream.cpp
)

add_library(fastOutput STATIC ${FASTOUTPUT_SRC})

target_include_directories(fastOutput PUBLIC
  ${FASTIO_DIR}/include
  ${COMMON_HEADER_DIR}
)

target_link_libraries(fastOutput PRIVATE
  common
  log
)

set_target_properties(fastOutput PROPERTIES FOLDER "common")

if(WIN32)
  add_subdirectory(traceIpc)
endif()



