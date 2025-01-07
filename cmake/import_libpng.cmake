# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED PNG_ABS_PATH)
  install_dependencies("--with-libpng")
  set(PNG_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpng")
endif()

add_definitions(-DPNG_ARM_NEON_OPT=0) 
add_library(png
  ${PNG_ABS_PATH}/png.c
  ${PNG_ABS_PATH}/pngerror.c
  ${PNG_ABS_PATH}/pngget.c
  ${PNG_ABS_PATH}/pngmem.c
  ${PNG_ABS_PATH}/pngpread.c
  ${PNG_ABS_PATH}/pngread.c
  ${PNG_ABS_PATH}/pngrio.c
  ${PNG_ABS_PATH}/pngrtran.c
  ${PNG_ABS_PATH}/pngrutil.c
  ${PNG_ABS_PATH}/pngset.c
  ${PNG_ABS_PATH}/pngtest.c
  ${PNG_ABS_PATH}/pngtrans.c
  ${PNG_ABS_PATH}/pngwio.c
  ${PNG_ABS_PATH}/pngwrite.c
  ${PNG_ABS_PATH}/pngwtran.c
  ${PNG_ABS_PATH}/pngwutil.c
)

include_directories(SYSTEM ${PNG_ABS_PATH})

set_target_properties(png PROPERTIES FOLDER External)