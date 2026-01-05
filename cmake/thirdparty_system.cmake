# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_property(GLOBAL PROPERTY IMPORT_ARGUMENT_LIST "")
set_property(GLOBAL PROPERTY IMPORT_SETUP_FUNC_LIST "")

function(add_thirdparty_arg arg_name)
  get_property(current_args GLOBAL PROPERTY IMPORT_ARGUMENT_LIST)
  list(APPEND current_args ${arg_name})
  set_property(GLOBAL PROPERTY IMPORT_ARGUMENT_LIST "${current_args}")
endfunction()

function(add_thirdparty_arg_setup arg_name func_name)
  add_thirdparty_arg(${arg_name})
  get_property(current_funcs GLOBAL PROPERTY IMPORT_SETUP_FUNC_LIST)
  list(APPEND current_funcs ${func_name})
  set_property(GLOBAL PROPERTY IMPORT_SETUP_FUNC_LIST "${current_funcs}")
endfunction()

function(install_and_setup_thirdparty)
  get_property(arg_list GLOBAL PROPERTY IMPORT_ARGUMENT_LIST)
  install_dependencies("${arg_list}")

  get_property(setup_funcs GLOBAL PROPERTY IMPORT_SETUP_FUNC_LIST)
  foreach(func IN LISTS setup_funcs)
    # For CMake 3.13 compatibility, we need to call functions differently
    # Generate and include a temporary CMake script to call each function
    set(temp_script "${CMAKE_BINARY_DIR}/temp_call_${func}.cmake")
    file(WRITE "${temp_script}" "${func}()\n")
    include("${temp_script}")
    file(REMOVE "${temp_script}")
  endforeach()
endfunction()