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

add_library(OpenGL_interceptor SHARED
  ${GENERATED_SRC}
  ${HAND_WRITTEN_SRC}
  ${CMAKE_SOURCE_DIR}/common/legacy/exception.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/dynamic_linker.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/gitsLoader.cpp
)

if(WIN32)
  target_sources(OpenGL_interceptor PRIVATE
    ${CMAKE_SOURCE_DIR}/common/legacy/tools_windows.cpp
  )
endif()

add_dependencies(OpenGL_interceptor OpenGL_codegen)

target_include_directories(OpenGL_interceptor PRIVATE ../recorder/include)

target_link_libraries(OpenGL_interceptor PRIVATE
  common_interface
  gits_recorder_interface
  log
)

if(WIN32)
  set_target_properties(OpenGL_interceptor PROPERTIES OUTPUT_NAME OpenGL32)
else()
  # libGL.so.1, loaded through LD_PRELOAD or LD_LIBRARY_PATH.
  set_target_properties(OpenGL_interceptor PROPERTIES OUTPUT_NAME GL SOVERSION 1)
  target_link_libraries(OpenGL_interceptor PRIVATE ${CMAKE_DL_LIBS})
endif()
set_target_properties(OpenGL_interceptor PROPERTIES FOLDER OpenGL)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenGL_interceptor> DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS OpenGL_interceptor
  RUNTIME DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR})
