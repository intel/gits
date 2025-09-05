# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(LOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LOG_HEADER_DIR ${LOG_SOURCE_DIR}/include)

set(LOG_SRC_HEADERS
  ${LOG_HEADER_DIR}/log2.h
)
set(LOG_SRC_SOURCES
  ${LOG_SOURCE_DIR}/log2.cpp
)

add_library(log STATIC ${LOG_SRC_HEADERS} ${LOG_SRC_SOURCES})

target_include_directories(log
PUBLIC
  ${PLOG_SOURCE_DIR}/include
  ${LOG_HEADER_DIR}
PRIVATE
  ${LOG_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../configuration/src/include
)

add_dependencies(log config_codegen)

set_target_properties(log PROPERTIES FOLDER "common")

# CCode
install(FILES ${LOG_SRC_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${LOG_SRC_HEADERS} DESTINATION CCode/CCodeSource/include)
# Copy Plog to CCode
install(
  DIRECTORY ${PLOG_SOURCE_DIR}/include
  DESTINATION CCode/third_party/plog
)

