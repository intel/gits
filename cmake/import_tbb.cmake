# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED TBB_DIR)
  add_thirdparty_arg_setup("--with-tbb" init_tbb)
  set(TBB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tbb/")
endif()

function(init_tbb)
  set(TBB_INSTALL OFF)
  set(TBB_TEST OFF)
  set(TBBMALLOC_BUILD OFF)
  add_definitions(-D__TBB_NO_IMPLICIT_LINKAGE=1)
  add_subdirectory(${TBB_DIR})

  # Rename the output to prevent conflicts with other TBB versions/copies in capture
  set_target_properties(tbb PROPERTIES OUTPUT_NAME gits_tbb)

  install(TARGETS tbb RUNTIME DESTINATION Recorder)
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:tbb> DESTINATION Recorder OPTIONAL)
  endif()
  install(FILES ${TBB_DIR}/LICENSE.txt DESTINATION Recorder RENAME LICENSE_tbb.txt)
endfunction()
