# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STUDIO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STUDIO_HEADER_DIR ${STUDIO_SOURCE_DIR}/include)

list(APPEND STUDIO_SOURCE
  ${STUDIO_SOURCE_DIR}/main.cpp
  ${STUDIO_SOURCE_DIR}/studioActions.cpp
  ${STUDIO_SOURCE_DIR}/studioApp.cpp
  ${STUDIO_SOURCE_DIR}/studioContext.cpp
  ${STUDIO_SOURCE_DIR}/studioConfig.cpp
  ${STUDIO_SOURCE_DIR}/controller.cpp
  ${STUDIO_SOURCE_DIR}/playbackSession.cpp

  ${STUDIO_HEADER_DIR}/common.h
  ${STUDIO_HEADER_DIR}/controller.h
  ${STUDIO_HEADER_DIR}/playbackSession.h
  ${STUDIO_HEADER_DIR}/resource.h
  ${STUDIO_HEADER_DIR}/studioActions.h
  ${STUDIO_HEADER_DIR}/studioApp.h
  ${STUDIO_HEADER_DIR}/studioContext.h
  ${STUDIO_HEADER_DIR}/studioConfig.h
  ${STUDIO_HEADER_DIR}/studioLabels.h
  ${STUDIO_HEADER_DIR}/studioEventBus.h
  )

list(APPEND STUDIO_PANEL_SOURCE
  ${STUDIO_SOURCE_DIR}/tracePanel.cpp
  ${STUDIO_SOURCE_DIR}/mainWindow.cpp
  ${STUDIO_SOURCE_DIR}/navBar.cpp
  ${STUDIO_SOURCE_DIR}/contentPanel.cpp
  ${STUDIO_SOURCE_DIR}/liveWorkloadPanel.cpp
  ${STUDIO_SOURCE_DIR}/streamPanel.cpp
)

list(APPEND STUDIO_PANEL_HEADER
  ${STUDIO_HEADER_DIR}/tracePanel.h
  ${STUDIO_HEADER_DIR}/mainWindow.h
  ${STUDIO_HEADER_DIR}/navBar.h
  ${STUDIO_HEADER_DIR}/contentPanel.h
  ${STUDIO_HEADER_DIR}/streamPanel.h
  ${STUDIO_HEADER_DIR}/liveWorkloadPanel.h
  )
source_group("Panels/Source files" FILES ${STUDIO_PANEL_SOURCE})
source_group("Panels/Header files" FILES ${STUDIO_PANEL_HEADER})

list(APPEND IMGUI_BACKEND_SOURCE
  ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
)
if(WIN64)
list(APPEND IMGUI_BACKEND_SOURCE
  ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
)
else()
list(APPEND IMGUI_BACKEND_SOURCE
  ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.cpp
)
endif()
source_group("ImGui-Backends" FILES ${IMGUI_BACKEND_SOURCE})

list(APPEND IMGUI_ADDONS_SOURCE
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.h

  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.cpp
  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.h
  ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialogConfig.h
)
source_group("ImGui-AddOns" FILES ${IMGUI_ADDONS_SOURCE})

# TODO Fix this, it's currently inside player folder.
# if(MSVC)
#   list(APPEND STUDIO_SOURCE
#     gitsLongPath.manifest) 
# endif()

add_executable(gits_studio ${STUDIO_SOURCE} ${STUDIO_PANEL_SOURCE} ${STUDIO_PANEL_HEADER}
  ${IMGUI_BACKEND_SOURCE} ${IMGUI_ADDONS_SOURCE} 
  ${STUDIO_SOURCE_DIR}/gitsStudio.rc
)
set_target_properties(gits_studio PROPERTIES OUTPUT_NAME "gitsStudio")
set_target_properties(gits_studio PROPERTIES FOLDER "${GUI_APPS_FOLDER}")

target_include_directories(gits_studio PRIVATE ${STUDIO_HEADER_DIR} 
  ${VULKAN_HEADERS_DIR}
  ${IMGUI_BACKEND_DIR} ${IMGUI_COLOR_TEXT_EDIT_DIR} ${IMGUI_FILE_DIALOG_DIR}
  ${PLOG_SOURCE_DIR}/include 
  ${COMMON_HEADER_DIR}
)

target_compile_options(gits_studio PRIVATE
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

target_compile_definitions(gits_studio PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

target_link_libraries(gits_studio PRIVATE imgui_common log common stream gui_common ${CMAKE_DL_LIBS})
gui_modules_link(gits_studio)
if(NOT WIN64)
target_link_libraries(gits_studio PRIVATE glfw)
endif()
add_dependencies(gits_studio config_codegen generate_font_header)
add_custom_command(TARGET gits_studio POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dxil-spirv-c-shared>
    $<TARGET_FILE_DIR:gits_studio>
)
