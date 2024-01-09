# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Boost's .cmake file depends on `CMAKE_SIZEOF_VOID_P` variable.
# Enable building 32 bit architecture on 64 bit host.
if(ARCH STREQUAL "-m32" OR CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
  set(CMAKE_SIZEOF_VOID_P 4)
  set(SCRIPT_ARG "--with-boost-32")
  set(BOOST_FOLDER "boost_x32")
else()
  set(SCRIPT_ARG "--with-boost-64")
  set(BOOST_FOLDER "boost_x64")
endif()

if(NOT DEFINED BOOST_ROOT)
  install_dependencies("${SCRIPT_ARG}")
  set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/${BOOST_FOLDER}")
endif()
add_definitions(-DBOOST_RATIO_EXTENSIONS)
if(MSVC)
  add_definitions(-DBOOST_ALL_NO_LIB)
endif()
add_definitions(-DBOOST_FILESYSTEM_SOURCE)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_DIR "${BOOST_ROOT}/lib/cmake/Boost-1.81.0")

find_package(Boost 1.81 CONFIG REQUIRED COMPONENTS container regex)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
