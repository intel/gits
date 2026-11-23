# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL_resource_dumping STATIC
  defaultFramebufferImage.h
  defaultFramebufferImage.cpp
  screenshotsLayer.h
  screenshotsLayer.cpp
)
add_dependencies(OpenGL_resource_dumping OpenGL_codegen)
target_include_directories(OpenGL_resource_dumping
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ../../player/include ${STB_DIR}
)
target_link_libraries(OpenGL_resource_dumping
  PUBLIC OpenGL_layer_interface
  PRIVATE stb_implementation log common_interface OpenGL_platform
)
set_target_properties(OpenGL_resource_dumping PROPERTIES FOLDER OpenGL)