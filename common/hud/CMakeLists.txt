# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(HUD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HUD_HEADER_DIR ${HUD_SOURCE_DIR}/include)

set(HUD_CPP_FILES
  ${HUD_SOURCE_DIR}/imGuiHUD.cpp
)

set(HUD_HEADER_FILES
  ${HUD_HEADER_DIR}/imGuiHUD.h
)

add_library(hud STATIC ${HUD_CPP_FILES} ${HUD_HEADER_FILES})

add_dependencies(hud configuration)

target_include_directories(
  hud PUBLIC
  ${HUD_HEADER_DIR}
)

set_target_properties(hud PROPERTIES FOLDER "common")

target_link_libraries(hud PRIVATE common configuration)
target_link_libraries(hud PUBLIC imgui_common)
