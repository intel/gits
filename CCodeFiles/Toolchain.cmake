# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================


set(toolchain_prefix $ENV{TOOLCHAIN_PREFIX})
if("${toolchain_prefix}" STREQUAL "")
message(FATAL_ERROR "Set TOOLCHAIN_PREFIX env variable")
endif()

string(REGEX REPLACE "/.?/.?$" "/sysroot/" toolchain_sysroot ${toolchain_prefix})

include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

CMAKE_FORCE_C_COMPILER(${toolchain_prefix}gcc GNU)
CMAKE_FORCE_CXX_COMPILER(${toolchain_prefix}g++ GNU)

set(CMAKE_FIND_ROOT_PATH ${toolchain_sysroot})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

