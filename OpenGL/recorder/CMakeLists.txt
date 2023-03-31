# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/openglRecorderWrapperAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openglRecorderWrapperAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openglRecorderWrapperIfaceAuto.h
  PROPERTIES GENERATED 1
)

add_library(OpenGL_recorder
  include/openglLibraryRecorder.h
  include/openglRecorderConditions.h
  include/openglRecorderPreSchedule.h
  include/openglRecorderSubWrappers.h
  include/openglRecorderWrapper.h
  include/openglRecorderWrapperAuto.h
  include/openglRecorderWrapperIface.h
  include/openglRecorderWrapperIfaceAuto.h
  include/openglState.h

  openglLibraryRecorder.cpp
  openglRecorderWrapper.cpp
  openglRecorderSubWrappers.cpp
  openglRecorderWrapperAuto.cpp
  openglState.cpp
)


source_group("Generated Files" REGULAR_EXPRESSION ".*Auto.*")

set_target_properties(OpenGL_recorder PROPERTIES FOLDER OpenGL)
