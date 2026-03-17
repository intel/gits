# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(STREAM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(STREAM_HEADER_DIR ${STREAM_SOURCE_DIR}/include)

set(STREAM_SRC_HEADERS
  ${STREAM_HEADER_DIR}/streamHeader.h
  ${STREAM_HEADER_DIR}/streamReader.h
  ${STREAM_HEADER_DIR}/streamLegacyReader.h
  ${STREAM_HEADER_DIR}/commandRunner.h
  ${STREAM_HEADER_DIR}/commandFactory.h
  ${STREAM_HEADER_DIR}/commandSerializer.h
  ${STREAM_HEADER_DIR}/streamWriter.h
  ${STREAM_HEADER_DIR}/commandId.h
  ${STREAM_HEADER_DIR}/streamCompressor.h
)
set(STREAM_SRC_SOURCES
  ${STREAM_SOURCE_DIR}/streamHeader.cpp
  ${STREAM_SOURCE_DIR}/streamReader.cpp
  ${STREAM_SOURCE_DIR}/streamLegacyReader.cpp
  ${STREAM_SOURCE_DIR}/streamWriter.cpp
  ${STREAM_SOURCE_DIR}/streamCompressor.cpp
)

add_library(stream STATIC ${STREAM_SRC_HEADERS} ${STREAM_SRC_SOURCES})

target_link_libraries(stream PRIVATE 
  log
  configuration
)

target_include_directories(stream
PUBLIC
  ${STREAM_HEADER_DIR}
PRIVATE
  ${STREAM_SOURCE_DIR}
)

set_target_properties(stream PROPERTIES FOLDER "common")
