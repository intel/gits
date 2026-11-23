# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Script mode (cmake -P): writes the PATCH, build metadata and commit of SOURCE_DIR into OUTPUT.
# Inputs: SOURCE_DIR, GIT_EXECUTABLE, RELEASE_BUILD, VERSION_PATCH, BUILD_ID, TEMPLATE, OUTPUT,
# PRINT_VERSION.

set(GITS_BUILD_ID_VALUE "")
set(GITS_BUILD_COMMIT "")
if(RELEASE_BUILD)
  set(GITS_VERSION_PATCH_VALUE "${VERSION_PATCH}")
else()
  set(GITS_VERSION_PATCH_VALUE 999)
  set(build_dirty "")
  if(GIT_EXECUTABLE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
                    WORKING_DIRECTORY "${SOURCE_DIR}"
                    RESULT_VARIABLE git_result
                    OUTPUT_VARIABLE GITS_BUILD_COMMIT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(git_result EQUAL 0 AND GITS_BUILD_COMMIT MATCHES "^[0-9a-f]+$")
      # --no-optional-locks: runs on every build, so it must not lock the index of the source tree.
      execute_process(COMMAND "${GIT_EXECUTABLE}" --no-optional-locks status --porcelain
                              --untracked-files=no
                      WORKING_DIRECTORY "${SOURCE_DIR}"
                      RESULT_VARIABLE git_result
                      OUTPUT_VARIABLE git_status
                      OUTPUT_STRIP_TRAILING_WHITESPACE
                      ERROR_QUIET)
      if(git_result EQUAL 0 AND NOT git_status STREQUAL "")
        set(build_dirty "-dirty")
      endif()
    else()
      set(GITS_BUILD_COMMIT "")
    endif()
  endif()

  if(NOT BUILD_ID STREQUAL "")
    set(GITS_BUILD_ID_VALUE "${BUILD_ID}")
  elseif(NOT GITS_BUILD_COMMIT STREQUAL "")
    string(SUBSTRING "${GITS_BUILD_COMMIT}" 0 7 commit_short)
    set(GITS_BUILD_ID_VALUE "local.sha-${commit_short}${build_dirty}")
  else()
    set(GITS_BUILD_ID_VALUE "local")
  endif()
endif()

set(previous_content "")
if(EXISTS "${OUTPUT}")
  file(READ "${OUTPUT}" previous_content)
endif()
# configure_file keeps the file and its timestamp when the content is unchanged.
configure_file("${TEMPLATE}" "${OUTPUT}" @ONLY)
file(READ "${OUTPUT}" content)

set(version_text "")
foreach(component MAJOR MINOR)
  file(STRINGS "${SOURCE_DIR}/common/legacy/include/gitsVersion.h" version_line
       REGEX "GITS_VERSION_${component} = [0-9]+")
  string(REGEX MATCH "GITS_VERSION_${component} = ([0-9]+)" version_match "${version_line}")
  string(APPEND version_text "${CMAKE_MATCH_1}.")
endforeach()
string(APPEND version_text "${GITS_VERSION_PATCH_VALUE}")
if(NOT RELEASE_BUILD)
  string(APPEND version_text "-dev+${GITS_BUILD_ID_VALUE}")
endif()
if(PRINT_VERSION)
  message(STATUS "GITS version: ${version_text}")
elseif(NOT content STREQUAL previous_content)
  message(STATUS "GITS version updated: ${version_text}")
endif()
