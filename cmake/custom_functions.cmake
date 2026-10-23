# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

function(install_dependencies args)
if(NOT SKIP_THIRDPARTY_FETCH)
  if(OFFLINE_MODE)
    set(args ${args} "--offline-mode")
  endif()
  execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/install_dependencies.py" ${args} RESULT_VARIABLE DEPENDENCIES_RESULT)
  if(NOT DEPENDENCIES_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not install external dependency.")
  endif()
endif()
endfunction()

# Recursively assign the VS/solution FOLDER of every non-interface target created
# under a source directory that was processed with add_subdirectory(). Used to
# keep RTSR-only third parties under gui_apps/External instead of the solution root.
function(gits_assign_folder_recursive source_dir folder)
  if(NOT IS_DIRECTORY "${source_dir}")
    return()
  endif()
  get_property(_gits_targets DIRECTORY "${source_dir}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_gits_target IN LISTS _gits_targets)
    get_target_property(_gits_type ${_gits_target} TYPE)
    if(_gits_type AND NOT _gits_type STREQUAL "INTERFACE_LIBRARY")
      set_target_properties(${_gits_target} PROPERTIES FOLDER "${folder}")
    endif()
  endforeach()
  get_property(_gits_subdirs DIRECTORY "${source_dir}" PROPERTY SUBDIRECTORIES)
  foreach(_gits_subdir IN LISTS _gits_subdirs)
    gits_assign_folder_recursive("${_gits_subdir}" "${folder}")
  endforeach()
endfunction()
