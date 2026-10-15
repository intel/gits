# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptorAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptorAuto.def
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

set(HAND_WRITTEN_SRC
  glTypes.h
  interceptorExports.h
  interceptorExports.cpp
  logging.h
  logging.cpp
  recorderLoader.h
  recorderLoader.cpp
  interceptorCustom.cpp
)

add_library(OpenGL2_interceptor SHARED
  ${GENERATED_SRC}
  ${HAND_WRITTEN_SRC}
  ${CMAKE_SOURCE_DIR}/common/legacy/exception.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/dynamic_linker.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/gitsLoader.cpp
)

if(WIN32)
  target_sources(OpenGL2_interceptor PRIVATE
    ${CMAKE_SOURCE_DIR}/common/legacy/tools_windows.cpp
  )
endif()

add_dependencies(OpenGL2_interceptor OpenGL2_codegen)

target_include_directories(OpenGL2_interceptor PRIVATE ../recorder/include)

target_link_libraries(OpenGL2_interceptor PRIVATE
  common_interface
  gits_recorder_interface
  log
)

set_target_properties(OpenGL2_interceptor PROPERTIES
  OUTPUT_NAME OpenGL32
  FOLDER OpenGL2
)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL2_interceptor> DESTINATION Recorder/${OpenGL2_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS OpenGL2_interceptor
  RUNTIME DESTINATION Recorder/${OpenGL2_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${OpenGL2_INSTALL_SUBDIR})
