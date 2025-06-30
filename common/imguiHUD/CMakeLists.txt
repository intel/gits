# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(IMGUIHUD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMGUIHUD_HEADER_DIR ${IMGUIHUD_SOURCE_DIR}/include)

set(IMGUIHUD_CPP_FILES
  ${IMGUIHUD_SOURCE_DIR}/imGuiHUD.cpp
  ${IMGUIHUD_SOURCE_DIR}/imGuiHelper.cpp
)

set(IMGUIHUD_FONT_C_HEADER ${IMGUIHUD_HEADER_DIR}/IntelOneMono-RegularAuto.h)
set(IMGUIHUD_FONT_CONVERT_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/scripts/convert_ttf_to_c_array.py)

set(IMGUIHUD_HEADER_FILES
  ${IMGUIHUD_HEADER_DIR}/imGuiHUD.h
  ${IMGUIHUD_HEADER_DIR}/imGuiHelper.h
  ${IMGUIHUD_FONT_C_HEADER}
)

set(IMGUI_SRC
  ${IMGUI_SOURCE_DIR}/imgui.cpp
  ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
  ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
  ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
  ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp

  ${IMGUI_SOURCE_DIR}/imgui.h
  ${IMGUI_SOURCE_DIR}/imconfig.h
  ${IMGUI_SOURCE_DIR}/imgui_internal.h
)
source_group("ImGui/" FILES ${IMGUI_SRC})

add_custom_command(
  OUTPUT ${IMGUIHUD_FONT_C_HEADER}
  COMMAND ${CMAKE_COMMAND} -E echo "Running Python script to generate font header..."
  COMMAND ${Python3_EXECUTABLE} ${IMGUIHUD_FONT_CONVERT_SCRIPT} ${INTELONEMONO_FONT_REGULAR} ${IMGUIHUD_FONT_C_HEADER}
  DEPENDS ${IMGUIHUD_FONT_CONVERT_SCRIPT}
  COMMENT "Generating C header from TTF font file"
)

add_custom_target(
  generateFontHeader ALL
  DEPENDS ${IMGUIHUD_FONT_TTF_FILE}
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(generateFontHeader PRIVATE
    ${IMGUIHUD_FONT_CONVERT_SCRIPT}
  )
endif()

add_library(ImGuiHUD STATIC ${IMGUIHUD_CPP_FILES} ${IMGUIHUD_HEADER_FILES} ${IMGUI_SRC})

add_dependencies(ImGuiHUD generateFontHeader configuration)

target_include_directories(
  ImGuiHUD PUBLIC
  ${IMGUIHUD_HEADER_DIR}
  ${IMGUI_SOURCE_DIR}/
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(ImGuiHUD PRIVATE
    ${IMGUI_SRC}
  )
endif()

set_target_properties(generateFontHeader PROPERTIES FOLDER "common/ImGuiHUD")
set_target_properties(ImGuiHUD PROPERTIES FOLDER "common/ImGuiHUD")

target_link_libraries(ImGuiHUD PUBLIC common configuration)