# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OPENGL_RECORDER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL_RECORDER_HEADER_DIR ${OPENGL_RECORDER_SOURCE_DIR}/include)

set_source_files_properties(
  ${OPENGL_RECORDER_SOURCE_DIR}/openglRecorderWrapperAuto.cpp
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapperAuto.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapperIfaceAuto.h
  PROPERTIES GENERATED 1
)

add_library(OpenGLLegacy_recorder
  ${OPENGL_RECORDER_HEADER_DIR}/openglLibraryRecorder.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderConditions.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderPreSchedule.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderSubWrappers.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapper.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapperAuto.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapperIface.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglRecorderWrapperIfaceAuto.h
  ${OPENGL_RECORDER_HEADER_DIR}/openglState.h

  ${OPENGL_RECORDER_SOURCE_DIR}/openglLibraryRecorder.cpp
  ${OPENGL_RECORDER_SOURCE_DIR}/openglRecorderWrapper.cpp
  ${OPENGL_RECORDER_SOURCE_DIR}/openglRecorderSubWrappers.cpp
  ${OPENGL_RECORDER_SOURCE_DIR}/openglRecorderWrapperAuto.cpp
  ${OPENGL_RECORDER_SOURCE_DIR}/openglState.cpp
)

target_include_directories(OpenGLLegacy_recorder PUBLIC ${OPENGL_RECORDER_HEADER_DIR})

add_library(OpenGLLegacy_recorder_interface INTERFACE)
target_include_directories(OpenGLLegacy_recorder_interface INTERFACE ${OPENGL_RECORDER_HEADER_DIR})

source_group("Generated Files" REGULAR_EXPRESSION ".*Auto.*")

set_target_properties(OpenGLLegacy_recorder PROPERTIES FOLDER OpenGLLegacy)

target_link_libraries(OpenGLLegacy_recorder PRIVATE gits_recorder_interface common configuration)
