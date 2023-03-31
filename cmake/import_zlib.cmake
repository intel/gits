# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED ZLIB_ABS_PATH)
  install_dependencies("--with-zlib")
  set(ZLIB_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib")
endif()

set(ZLIB_MINIZIP_ABS_PATH "${ZLIB_ABS_PATH}/contrib/minizip")

include_directories(SYSTEM ${ZLIB_ABS_PATH} ${ZLIB_MINIZIP_ABS_PATH})

add_library(zlib
  ${ZLIB_ABS_PATH}/adler32.c
  ${ZLIB_ABS_PATH}/compress.c
  ${ZLIB_ABS_PATH}/crc32.c
  ${ZLIB_ABS_PATH}/deflate.c
  ${ZLIB_ABS_PATH}/gzclose.c
  ${ZLIB_ABS_PATH}/gzlib.c
  ${ZLIB_ABS_PATH}/gzread.c
  ${ZLIB_ABS_PATH}/gzwrite.c
  ${ZLIB_ABS_PATH}/infback.c
  ${ZLIB_ABS_PATH}/inffast.c
  ${ZLIB_ABS_PATH}/inflate.c
  ${ZLIB_ABS_PATH}/inftrees.c
  ${ZLIB_ABS_PATH}/trees.c
  ${ZLIB_ABS_PATH}/uncompr.c
  ${ZLIB_ABS_PATH}/zutil.c
  ${ZLIB_ABS_PATH}/contrib/minizip/ioapi.c
  ${ZLIB_ABS_PATH}/contrib/minizip/miniunz.c
  ${ZLIB_ABS_PATH}/contrib/minizip/minizip.c
  ${ZLIB_ABS_PATH}/contrib/minizip/mztools.c
  ${ZLIB_ABS_PATH}/contrib/minizip/unzip.c
  ${ZLIB_ABS_PATH}/contrib/minizip/zip.c
)

set_target_properties(zlib PROPERTIES FOLDER External)
