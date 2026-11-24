# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/glDispatchAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/glDispatchAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

# Embed the object in each owner's archive alongside its driver-export hook.
add_library(OpenGL_dispatch OBJECT ${GENERATED_SRC} driverExports.h)
add_dependencies(OpenGL_dispatch OpenGL_codegen)
target_include_directories(OpenGL_dispatch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(OpenGL_dispatch PRIVATE OpenGL_layer_interface log)
set_target_properties(OpenGL_dispatch PROPERTIES FOLDER OpenGL)
