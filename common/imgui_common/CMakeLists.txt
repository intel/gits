# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(IMGUILIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMGUILIB_HEADER_DIR ${IMGUILIB_SOURCE_DIR}/include)

# Font
set(IMGUILIB_FONT_C_HEADER ${IMGUILIB_HEADER_DIR}/IntelOneMono-RegularAuto.h)
set(IMGUILIB_FONT_CONVERT_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/scripts/convert_ttf_to_c_array.py)

add_custom_command(
  OUTPUT ${IMGUILIB_FONT_C_HEADER}
  COMMAND ${CMAKE_COMMAND} -E echo "Running Python script to generate font header..."
  COMMAND ${Python3_EXECUTABLE} ${IMGUILIB_FONT_CONVERT_SCRIPT} ${INTELONEMONO_FONT_REGULAR} ${IMGUILIB_FONT_C_HEADER}
  DEPENDS ${IMGUILIB_FONT_CONVERT_SCRIPT}
  COMMENT "Generating C header from TTF font file"
)

add_custom_target(
  generate_font_header ALL
  DEPENDS ${IMGUILIB_FONT_TTF_HEADER}
)
set_target_properties(generate_font_header PROPERTIES FOLDER "common/imgui")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(generate_font_header PRIVATE
    ${IMGUILIB_FONT_CONVERT_SCRIPT}
  )
endif()

# ImGui Lib
set(IMGUILIB_CPP_FILES
  ${IMGUILIB_SOURCE_DIR}/imGuiHelper.cpp
  ${IMGUILIB_SOURCE_DIR}/imGuiStyle.cpp
  ${IMGUILIB_SOURCE_DIR}/textEditorWidget.cpp
)

set(IMGUILIB_HEADER_FILES
  ${IMGUILIB_HEADER_DIR}/imGuiStyle.h
  ${IMGUILIB_HEADER_DIR}/imGuiHelper.h
  ${IMGUILIB_HEADER_DIR}/textEditorWidget.h
  ${IMGUILIB_HEADER_DIR}/baseButtonGroup.h
  ${IMGUILIB_HEADER_DIR}/buttonGroup.h
  ${IMGUILIB_HEADER_DIR}/tabGroup.h
  ${IMGUILIB_FONT_C_HEADER}
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

add_library(imgui_common STATIC ${IMGUILIB_CPP_FILES} ${IMGUILIB_HEADER_FILES} ${IMGUI_SRC})
set_target_properties(imgui_common PROPERTIES FOLDER "common/imgui")

target_include_directories(
  imgui_common PUBLIC
  ${IMGUILIB_HEADER_DIR}
  ${IMGUI_SOURCE_DIR}/
  ${IMGUI_COLOR_TEXT_EDIT_DIR} 
)

target_include_directories(
  imgui_common INTERFACE
  ${IMGUI_SOURCE_DIR}/
  ${IMGUI_BACKEND_DIR}/
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(imgui_common PRIVATE
    ${IMGUI_SRC}
  )
endif()

target_link_libraries(imgui_common PRIVATE yamlcpp log)
add_dependencies(imgui_common generate_font_header config_codegen)

list(APPEND IMGUI_ADDONS_SOURCE
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.h

  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.cpp
  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.h
  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialogConfig.h
)
add_library(imgui_addons STATIC ${IMGUI_ADDONS_SOURCE})
target_link_libraries(imgui_addons PRIVATE imgui_common)
set_target_properties(imgui_addons PROPERTIES FOLDER "common/imgui")

target_compile_options(imgui_addons PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -Wno-deprecated-enum-enum-conversion
    -Wno-error=reorder
    -Wno-error=sign-compare
    -Wno-error=parentheses
    -Wno-error=sequence-point
    -Wno-error=unknown-pragmas
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /wd4996  # deprecated warnings
    /wd4244  # conversion warnings
    /wd4267  # size_t conversion warnings
    /wd4018  # signed/unsigned mismatch
  >
)

