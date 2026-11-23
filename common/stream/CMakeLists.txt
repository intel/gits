# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(STREAM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(STREAM_HEADER_DIR ${STREAM_SOURCE_DIR}/include)

if(NOT "${ARCH}" STREQUAL "-m32")
  set(TBB_DEPENDENT_HEADERS
    ${STREAM_HEADER_DIR}/orderingRecorder.h
    ${STREAM_HEADER_DIR}/recordingSession.h
  )
  set(TBB_DEPENDENT_SOURCES
    ${STREAM_SOURCE_DIR}/orderingRecorder.cpp
    ${STREAM_SOURCE_DIR}/recordingSession.cpp
  )
  set(TBB_DEPENDENT_LIBRARIES tbb)
endif()

set(STREAM_SRC_HEADERS
  ${STREAM_HEADER_DIR}/streamHeader.h
  ${STREAM_HEADER_DIR}/streamReader.h
  ${STREAM_HEADER_DIR}/streamLegacyReader.h
  ${STREAM_HEADER_DIR}/commandRunner.h
  ${STREAM_HEADER_DIR}/commandFactory.h
  ${STREAM_HEADER_DIR}/commandSerializer.h
  ${STREAM_HEADER_DIR}/streamWriter.h
  ${STREAM_HEADER_DIR}/commandId.h
  ${STREAM_HEADER_DIR}/streamCompressor.h
  ${STREAM_HEADER_DIR}/diskSpaceCheck.h
  ${STREAM_HEADER_DIR}/externalHandleService.h
  ${STREAM_HEADER_DIR}/subcaptureSession.h
  ${STREAM_HEADER_DIR}/restoredContents.h
  ${TBB_DEPENDENT_HEADERS}
)
set(STREAM_SRC_SOURCES
  ${STREAM_SOURCE_DIR}/streamHeader.cpp
  ${STREAM_SOURCE_DIR}/streamReader.cpp
  ${STREAM_SOURCE_DIR}/streamLegacyReader.cpp
  ${STREAM_SOURCE_DIR}/streamWriter.cpp
  ${STREAM_SOURCE_DIR}/streamCompressor.cpp
  ${STREAM_SOURCE_DIR}/diskSpaceCheck.cpp
  ${STREAM_SOURCE_DIR}/externalHandleService.cpp
  ${STREAM_SOURCE_DIR}/subcaptureSession.cpp
  ${TBB_DEPENDENT_SOURCES}
)

add_library(stream STATIC ${STREAM_SRC_HEADERS} ${STREAM_SRC_SOURCES})

target_link_libraries(stream PRIVATE 
  log
  configuration
  yamlcpp
  lz4
  libzstd_static
  ${TBB_DEPENDENT_LIBRARIES}
)

target_include_directories(stream
PUBLIC
  ${STREAM_HEADER_DIR}
PRIVATE
  ${STREAM_SOURCE_DIR}
)

set_target_properties(stream PROPERTIES FOLDER "common")

# Host-only gate for the analysis files a subcapture recording pass can use, on which the APIs of
# a stream decide together. Run the executable; it prints failures and exits non-zero.
add_executable(stream_subcapture_analysis_test
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/subcaptureAnalysisTest.cpp
)
target_link_libraries(stream_subcapture_analysis_test PRIVATE stream configuration log)
set_target_properties(stream_subcapture_analysis_test PROPERTIES FOLDER "common")

# Host-only gate for the check that the exporting API restores the contents an import uses: no
# GPU, no driver, no stream. Run the executable; it prints failures and exits non-zero. It needs
# only the inline functions of restoredContents.h, so it links nothing.
add_executable(stream_restored_contents_test
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/restoredContentsTest.cpp
)
target_include_directories(stream_restored_contents_test PRIVATE
  ${STREAM_HEADER_DIR}
)

set_target_properties(stream_restored_contents_test PROPERTIES FOLDER "common")

# Host-only gate for stopping the stream reader: by an exception of a command or by closing it
# while every block ahead of the running command is full, and by the end of the stream or by
# closing it while the running command waits for a block. Run the executable; it prints failures
# and exits non-zero, also when the reader hangs.
add_executable(stream_reader_test
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/streamReaderTest.cpp
)
target_link_libraries(stream_reader_test PRIVATE stream configuration log)
set_target_properties(stream_reader_test PROPERTIES FOLDER "common")
