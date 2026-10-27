# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(OpenGL2_platform STATIC
  systemApi.h
  systemApi.cpp
)

target_include_directories(OpenGL2_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
  target_link_libraries(OpenGL2_platform PRIVATE user32)
endif()

set_target_properties(OpenGL2_platform PROPERTIES FOLDER OpenGL2/OpenGL2_common)
