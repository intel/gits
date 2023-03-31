# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED CL_HEADERS_ROOT)
  install_dependencies("--with-clheaders")
  set(CL_HEADERS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenCL-Headers")
endif()


add_definitions(-DCL_TARGET_OPENCL_VERSION=300)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_0_APIS)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_1_APIS)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_2_APIS)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_2_0_APIS)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_2_1_APIS)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_2_2_APIS)

include_directories(${CL_HEADERS_ROOT})
