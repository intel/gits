# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED STACKWALKER_ABS_PATH)
  add_thirdparty_arg_setup("--with-stackwalker" init_stackwalker)
  set(STACKWALKER_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/StackWalker/Main/StackWalker")
endif()

function(init_stackwalker)
  add_library(stackwalker
    ${STACKWALKER_ABS_PATH}/StackWalker.cpp
  )

  source_group("External Files" FILES
    ${STACKWALKER_ABS_PATH}/StackWalker.cpp
    ${STACKWALKER_ABS_PATH}/StackWalker.h
  )
  set_target_properties(stackwalker PROPERTIES FOLDER External)

  include_directories(${STACKWALKER_ABS_PATH})
endfunction()