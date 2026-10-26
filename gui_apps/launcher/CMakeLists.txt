# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(LAUNCHER_SOURCE_DIR ${GUI_APPS_DIR}/launcher)
set(LAUNCHER_HEADER_DIR ${LAUNCHER_SOURCE_DIR}/include)

list(APPEND LAUNCHER_SOURCE
  ${LAUNCHER_SOURCE_DIR}/main.cpp
  ${LAUNCHER_SOURCE_DIR}/launcherActions.cpp
  ${LAUNCHER_SOURCE_DIR}/launcherApp.cpp
  ${LAUNCHER_SOURCE_DIR}/context.cpp
  ${LAUNCHER_SOURCE_DIR}/configOptionsGuiHelpers.cpp

  ${LAUNCHER_HEADER_DIR}/launcherCommon.h
  ${LAUNCHER_HEADER_DIR}/launcherActions.h
  ${LAUNCHER_HEADER_DIR}/launcherApp.h
  ${LAUNCHER_HEADER_DIR}/launcherConfig.h
  ${LAUNCHER_HEADER_DIR}/context.h
  ${LAUNCHER_HEADER_DIR}/resource.h
  ${LAUNCHER_HEADER_DIR}/labels.h
  ${LAUNCHER_HEADER_DIR}/configOptionsGuiHelpers.h
  )

list(APPEND LAUNCHER_PANEL_SOURCE
  ${LAUNCHER_SOURCE_DIR}/contentPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/mainWindow.cpp
  ${LAUNCHER_SOURCE_DIR}/playbackPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/capturePanel.cpp
  ${LAUNCHER_SOURCE_DIR}/captureOptionsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/playbackOptionsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/subcaptureOptionsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/resourceDumpPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/pluginsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/optionsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/yamlPanel.cpp

  ${LAUNCHER_HEADER_DIR}/contentPanel.h
  ${LAUNCHER_HEADER_DIR}/captureOptionsPanel.h
  ${LAUNCHER_HEADER_DIR}/playbackOptionsPanel.h
  ${LAUNCHER_HEADER_DIR}/subcaptureOptionsPanel.h
  ${LAUNCHER_HEADER_DIR}/resourceDumpPanel.h
  ${LAUNCHER_HEADER_DIR}/mainWindow.h
  ${LAUNCHER_HEADER_DIR}/playbackPanel.h
  ${LAUNCHER_HEADER_DIR}/capturePanel.h
  ${LAUNCHER_HEADER_DIR}/pluginsPanel.h
  ${LAUNCHER_HEADER_DIR}/optionsPanel.h
  ${LAUNCHER_HEADER_DIR}/yamlPanel.h
  )

if(WIN64)
list(APPEND LAUNCHER_SOURCE
  ${LAUNCHER_SOURCE_DIR}/registryManager.cpp

  ${LAUNCHER_HEADER_DIR}/registryManager.h
)

list(APPEND LAUNCHER_PANEL_SOURCE
  ${LAUNCHER_SOURCE_DIR}/systemSetupPanel.cpp

  ${LAUNCHER_HEADER_DIR}/systemSetupPanel.h
)
endif()

source_group("Panels" FILES ${LAUNCHER_PANEL_SOURCE})

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
#   list(APPEND LAUNCHER_SOURCE
#     gitsLongPath.manifest) 
# endif()

add_executable(gits_launcher ${LAUNCHER_SOURCE} ${LAUNCHER_PANEL_SOURCE}
  ${IMGUI_BACKEND_SOURCE} ${IMGUI_ADDONS_SOURCE} 
  ${LAUNCHER_SOURCE_DIR}/gitsLauncher.rc
)
set_target_properties(gits_launcher PROPERTIES OUTPUT_NAME "gitsLauncher")
set_target_properties(gits_launcher PROPERTIES FOLDER "${GUI_APPS_FOLDER}")

target_include_directories(gits_launcher PRIVATE ${LAUNCHER_HEADER_DIR}
  ${VULKAN_HEADERS_DIR}
  ${IMGUI_BACKEND_DIR} ${IMGUI_COLOR_TEXT_EDIT_DIR} ${IMGUI_FILE_DIALOG_DIR}
  ${PLOG_SOURCE_DIR}/include 
  ${COMMON_HEADER_DIR}
)

target_compile_options(gits_launcher PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -Wno-deprecated-enum-enum-conversion
    -Wno-error=reorder
    -Wno-error=sign-compare
    -Wno-error=parentheses
    -Wno-error=sequence-point
    -Wno-error=unknown-pragmas
    -Wno-error=unused-but-set-variable
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /wd4996  # deprecated warnings
    /wd4244  # conversion warnings
    /wd4267  # size_t conversion warnings
    /wd4018  # signed/unsigned mismatch
  >
)

target_compile_definitions(gits_launcher PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

target_link_libraries(gits_launcher PRIVATE imgui_common log common stream ${CMAKE_DL_LIBS} gui_common)

if(NOT WIN64)
target_link_libraries(gits_launcher PRIVATE glfw X11)
endif()
add_dependencies(gits_launcher config_codegen generate_font_header)
add_custom_command(TARGET gits_launcher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dxil-spirv-c-shared>
    $<TARGET_FILE_DIR:gits_launcher>
)

install(TARGETS gits_launcher
  DESTINATION .
  COMPONENT gits_launcher
)
if(WIN64)
  install(FILES ${LAUNCHER_SOURCE_DIR}/registryKeys.yml
    DESTINATION Launcher
    COMPONENT gits_launcher
  )
endif()
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_launcher>
    DESTINATION .
    COMPONENT gits_launcher
    OPTIONAL
  )
endif()




