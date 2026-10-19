# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

function(gui_module_register target id)
  set_property(GLOBAL APPEND PROPERTY GUI_MODULE_TARGETS ${target})
  set_property(GLOBAL APPEND PROPERTY GUI_MODULE_SYMBOLS gitsGuiModule_${id})
endfunction()

function(gui_modules_link target)
  get_property(module_targets GLOBAL PROPERTY GUI_MODULE_TARGETS)
  get_property(module_symbols GLOBAL PROPERTY GUI_MODULE_SYMBOLS)
  target_link_libraries(${target} PRIVATE ${module_targets})
  foreach(symbol IN LISTS module_symbols)
    if(MSVC)
      target_link_options(${target} PRIVATE "/INCLUDE:${symbol}")
    else()
      target_link_options(${target} PRIVATE "-u" "${symbol}")
    endif()
  endforeach()
endfunction()

add_subdirectory(common)
add_subdirectory(rtsr)
