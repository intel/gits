# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED MURMURHASH_ABS_PATH)
  install_dependencies("--with-murmurhash")
  set(MURMURHASH_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/MurmurHash/src")
endif()

add_library(murmurhash
  ${MURMURHASH_ABS_PATH}/MurmurHash3.cpp
  ${MURMURHASH_ABS_PATH}/MurmurHash3.h
)

source_group("External Files" FILES
  ${MURMURHASH_ABS_PATH}/MurmurHash3.cpp
  ${MURMURHASH_ABS_PATH}/MurmurHash3.h
)

set_target_properties(murmurhash PROPERTIES FOLDER External)

include_directories(SYSTEM ${MURMURHASH_ABS_PATH})
