# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL2_resource_dumping STATIC
  screenshotsLayer.h
  screenshotsLayer.cpp
)
add_dependencies(OpenGL2_resource_dumping OpenGL2_codegen)
target_include_directories(OpenGL2_resource_dumping
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ../../player/include ${STB_DIR}
)
target_link_libraries(OpenGL2_resource_dumping
  PUBLIC OpenGL2_layer_interface
  PRIVATE stb_implementation log common_interface
)
set_target_properties(OpenGL2_resource_dumping PROPERTIES FOLDER OpenGL2)