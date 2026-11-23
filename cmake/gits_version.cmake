# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# GITS version, see docs/development/versioning.md. MAJOR and MINOR come from
# common/legacy/include/gitsVersion.h; this file selects PATCH and the build metadata.
option(GITS_RELEASE_BUILD "Release build: PATCH from GITS_VERSION_PATCH, no build metadata" OFF)
set(GITS_VERSION_PATCH "" CACHE STRING "PATCH of a release build (0..998); must be empty otherwise")
set(GITS_BUILD_ID "" CACHE STRING
  "SemVer build metadata of a development build; empty selects local.sha-<commit>[-dirty]")

if(GITS_RELEASE_BUILD)
  if(NOT GITS_VERSION_PATCH MATCHES "^(0|[1-9][0-9]*)$" OR GITS_VERSION_PATCH GREATER 998)
    message(FATAL_ERROR
      "GITS_RELEASE_BUILD requires GITS_VERSION_PATCH between 0 and 998, got '${GITS_VERSION_PATCH}'")
  endif()
  if(NOT GITS_BUILD_ID STREQUAL "")
    message(FATAL_ERROR "GITS_BUILD_ID must be empty when GITS_RELEASE_BUILD is ON")
  endif()
elseif(NOT GITS_VERSION_PATCH STREQUAL "")
  message(FATAL_ERROR "GITS_VERSION_PATCH is only allowed when GITS_RELEASE_BUILD is ON")
endif()
# SemVer build metadata: dot-separated, non-empty identifiers of [0-9A-Za-z-]. It is also part of
# development package names.
if(NOT GITS_BUILD_ID STREQUAL "" AND NOT GITS_BUILD_ID MATCHES "^[0-9A-Za-z-]+(\\.[0-9A-Za-z-]+)*$")
  message(FATAL_ERROR
    "GITS_BUILD_ID must be dot-separated identifiers of letters, digits and -, got '${GITS_BUILD_ID}'")
endif()

find_package(Git QUIET)
set(GITS_BUILD_INFO_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/buildInfo")
set(gits_build_info_command
    "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}"
    "-DRELEASE_BUILD=${GITS_RELEASE_BUILD}"
    "-DVERSION_PATCH=${GITS_VERSION_PATCH}"
    "-DBUILD_ID=${GITS_BUILD_ID}"
    "-DTEMPLATE=${CMAKE_CURRENT_LIST_DIR}/gitsBuildInfo.h.in"
    "-DOUTPUT=${GITS_BUILD_INFO_INCLUDE_DIR}/gitsBuildInfo.h")

execute_process(COMMAND ${gits_build_info_command} -DPRINT_VERSION=ON
                        -P "${CMAKE_CURRENT_LIST_DIR}/write_build_info.cmake"
                RESULT_VARIABLE gits_build_info_result)
if(NOT gits_build_info_result EQUAL 0)
  message(FATAL_ERROR "Failed to write ${GITS_BUILD_INFO_INCLUDE_DIR}/gitsBuildInfo.h")
endif()

# Runs on every build so the commit and dirty state stay current. Only version.cpp includes the
# header, and it is rewritten only when its content changes.
add_custom_target(gits_build_info
                  COMMAND ${gits_build_info_command}
                          -P "${CMAKE_CURRENT_LIST_DIR}/write_build_info.cmake"
                  BYPRODUCTS "${GITS_BUILD_INFO_INCLUDE_DIR}/gitsBuildInfo.h"
                  VERBATIM)
set_target_properties(gits_build_info PROPERTIES FOLDER "common")
