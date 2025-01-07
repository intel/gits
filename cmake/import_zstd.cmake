# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED ZSTD_ABS_PATH)
  install_dependencies("--with-zstd")
  set(ZSTD_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/zstd/lib")
endif()

# Define library directory, where sources and header files are located
include_directories(${ZSTD_ABS_PATH} ${ZSTD_ABS_PATH}/common)

file(GLOB CommonSources ${ZSTD_ABS_PATH}/common/*.c)
file(GLOB CompressSources ${ZSTD_ABS_PATH}/compress/*.c)
if (MSVC)
    file(GLOB DecompressSources ${ZSTD_ABS_PATH}/decompress/*.c)
    add_compile_options(-DZSTD_DISABLE_ASM)
else ()
    file(GLOB DecompressSources ${ZSTD_ABS_PATH}/decompress/*.c ${ZSTD_ABS_PATH}/decompress/*.S)
endif ()
file(GLOB DictBuilderSources ${ZSTD_ABS_PATH}/dictBuilder/*.c)

set(Sources
        ${CommonSources}
        ${CompressSources}
        ${DecompressSources}
        ${DictBuilderSources})

file(GLOB CommonHeaders ${ZSTD_ABS_PATH}/common/*.h)
file(GLOB CompressHeaders ${ZSTD_ABS_PATH}/compress/*.h)
file(GLOB DecompressHeaders ${ZSTD_ABS_PATH}/decompress/*.h)
file(GLOB DictBuilderHeaders ${ZSTD_ABS_PATH}/dictBuilder/*.h)

set(Headers
        ${ZSTD_ABS_PATH}/zstd.h
        ${CommonHeaders}
        ${CompressHeaders}
        ${DecompressHeaders}
        ${DictBuilderHeaders})

# Explicitly set the language to C for all files, including ASM files.
# Our assembly expects to be compiled by a C compiler, and is only enabled for
# __GNUC__ compatible compilers. Otherwise all the ASM code is disabled by
# macros.
set_source_files_properties(${Sources} PROPERTIES LANGUAGE C)

add_library(libzstd_static STATIC ${Sources} ${Headers})

# Add specific compile definitions for MSVC project
if (MSVC)
    set_property(TARGET libzstd_static APPEND PROPERTY COMPILE_DEFINITIONS "ZSTD_HEAPMODE=0;_CRT_SECURE_NO_WARNINGS")
endif ()

# With MSVC static library needs to be renamed to avoid conflict with import library
if (MSVC OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MINGW))
    set(STATIC_LIBRARY_BASE_NAME zstd_static)
else ()
    set(STATIC_LIBRARY_BASE_NAME zstd)
endif ()

set_target_properties(
        libzstd_static
        PROPERTIES
        FOLDER External
        POSITION_INDEPENDENT_CODE On
        OUTPUT_NAME ${STATIC_LIBRARY_BASE_NAME})