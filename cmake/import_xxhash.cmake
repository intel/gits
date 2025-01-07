# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED XXHASH_ABS_PATH)
  install_dependencies("--with-xxhash")
  set(XXHASH_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xxhash")
endif()

add_definitions(-DXXH_INLINE_ALL)

add_library(xxhash
  ${XXHASH_ABS_PATH}/xxhash.c
  ${XXHASH_ABS_PATH}/xxhash.h
)

set_target_properties(xxhash PROPERTIES FOLDER External)


include_directories(${XXHASH_ABS_PATH})
