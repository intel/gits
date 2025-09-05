# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

function(install_dependencies args)
if(NOT SKIP_THIRDPARTY_FETCH)
  set(args ${args} "--meta-path" ${CMAKE_BINARY_DIR})
  execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/install_dependencies.py" ${args} RESULT_VARIABLE DEPENDENCIES_RESULT)
  if(NOT DEPENDENCIES_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not install external dependency.")
  endif()
endif()
endfunction()
