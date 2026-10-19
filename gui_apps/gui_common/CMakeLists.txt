# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

set(GUI_COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(GUI_COMMON_HEADER_DIR ${GUI_COMMON_SOURCE_DIR}/include)


# GUI Common
set(IMGUI_APP_CPP_FILES
  ${GUI_COMMON_SOURCE_DIR}/appPaths.cpp
  ${GUI_COMMON_SOURCE_DIR}/baseConfig.cpp
  ${GUI_COMMON_SOURCE_DIR}/baseContext.cpp
  ${GUI_COMMON_SOURCE_DIR}/contextHelper.cpp
  ${GUI_COMMON_SOURCE_DIR}/configOptions.cpp
  ${GUI_COMMON_SOURCE_DIR}/imGuiApp.cpp
  ${GUI_COMMON_SOURCE_DIR}/vkWindow.cpp
  ${GUI_COMMON_SOURCE_DIR}/vulkanLoader.cpp
  ${GUI_COMMON_SOURCE_DIR}/yamlUtils.cpp
)
source_group("ImGui App/Source Files" FILES ${IMGUI_APP_CPP_FILES})

set(IMGUI_APP_HEADER_FILES
  ${GUI_COMMON_HEADER_DIR}/appPaths.h
  ${GUI_COMMON_HEADER_DIR}/baseCommon.h
  ${GUI_COMMON_HEADER_DIR}/baseConfig.h
  ${GUI_COMMON_HEADER_DIR}/baseContext.h
  ${GUI_COMMON_HEADER_DIR}/baseLabels.h
  ${GUI_COMMON_HEADER_DIR}/basePanel.h
  ${GUI_COMMON_HEADER_DIR}/baseEventBus.h
  ${GUI_COMMON_HEADER_DIR}/contextHelper.h
  ${GUI_COMMON_HEADER_DIR}/configOptions.h
  ${GUI_COMMON_HEADER_DIR}/imGuiApp.h
  ${GUI_COMMON_HEADER_DIR}/vkWindow.h
  ${GUI_COMMON_HEADER_DIR}/vulkanFunctions.inl
  ${GUI_COMMON_HEADER_DIR}/vulkanLoader.h
  ${GUI_COMMON_HEADER_DIR}/yamlUtils.h
  ${GUI_COMMON_HEADER_DIR}/windowEvents.h
)
source_group("ImGui App/Header Files" FILES ${IMGUI_APP_HEADER_FILES})

set(SHARED_PANELS_FILES
${GUI_COMMON_SOURCE_DIR}/metaDataPanel.cpp

${GUI_COMMON_HEADER_DIR}/metaDataPanel.h
)
source_group("Shared Panels" FILES ${SHARED_PANELS_FILES})

set(GUI_ACTIONS_SOURCE_FILES
${GUI_COMMON_SOURCE_DIR}/captureActions.cpp
${GUI_COMMON_SOURCE_DIR}/fileActions.cpp
${GUI_COMMON_SOURCE_DIR}/guiActions.cpp
${GUI_COMMON_SOURCE_DIR}/metaDataActions.cpp
)
set(GUI_ACTIONS_HEADER_FILES
${GUI_COMMON_HEADER_DIR}/captureActions.h
${GUI_COMMON_HEADER_DIR}/fileActions.h
${GUI_COMMON_HEADER_DIR}/guiActions.h
${GUI_COMMON_HEADER_DIR}/metaDataActions.h
)
source_group("Actions/Source Files" FILES ${GUI_ACTIONS_SOURCE_FILES})
source_group("Actions/Header Files" FILES ${GUI_ACTIONS_HEADER_FILES})

add_library(gui_common STATIC ${IMGUI_APP_CPP_FILES} ${IMGUI_APP_HEADER_FILES} ${SHARED_PANELS_FILES} ${GUI_ACTIONS_SOURCE_FILES} ${GUI_ACTIONS_HEADER_FILES})
set_target_properties(gui_common PROPERTIES FOLDER "${GUI_APPS_FOLDER}")

target_include_directories(
  gui_common PRIVATE
  ${VULKAN_HEADERS_DIR}
  ${IMGUI_FILE_DIALOG_DIR}
)

target_include_directories(
  gui_common PUBLIC
  ${GUI_COMMON_HEADER_DIR}
)

target_include_directories(
  gui_common INTERFACE
  ${GUI_COMMON_HEADER_DIR}
)

target_link_libraries(gui_common PRIVATE yamlcpp log imgui_common common stream)
add_dependencies(gui_common config_codegen)
