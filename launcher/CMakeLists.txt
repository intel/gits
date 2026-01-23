set(LAUNCHER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LAUNCHER_HEADER_DIR ${LAUNCHER_SOURCE_DIR}/include)

find_package(Vulkan REQUIRED)

list(APPEND LAUNCHER_SOURCE
  ${LAUNCHER_SOURCE_DIR}/fileActions.cpp
  ${LAUNCHER_SOURCE_DIR}/main.cpp
  ${LAUNCHER_SOURCE_DIR}/guiController.cpp
  ${LAUNCHER_SOURCE_DIR}/launcherActions.cpp
  ${LAUNCHER_SOURCE_DIR}/launcherConfig.cpp
  ${LAUNCHER_SOURCE_DIR}/captureActions.cpp
  ${LAUNCHER_SOURCE_DIR}/context.cpp
  ${LAUNCHER_SOURCE_DIR}/metaDataActions.cpp

  ${LAUNCHER_HEADER_DIR}/fileActions.h
  ${LAUNCHER_HEADER_DIR}/launcherActions.h
  ${LAUNCHER_HEADER_DIR}/launcherConfig.h
  ${LAUNCHER_HEADER_DIR}/context.h
  ${LAUNCHER_HEADER_DIR}/resource.h
  ${LAUNCHER_HEADER_DIR}/labels.h
  ${LAUNCHER_HEADER_DIR}/guiController.h
  ${LAUNCHER_HEADER_DIR}/captureActions.h
  ${LAUNCHER_HEADER_DIR}/launcherFormatter.h
  ${LAUNCHER_HEADER_DIR}/metaDataActions.h
)

list(APPEND LAUNCHER_PANEL_SOURCE
  ${LAUNCHER_SOURCE_DIR}/contentPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/mainWindow.cpp
  ${LAUNCHER_SOURCE_DIR}/playbackPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/capturePanel.cpp
  ${LAUNCHER_SOURCE_DIR}/ezOptionsPanel.cpp
  ${LAUNCHER_SOURCE_DIR}/subcapturePanel.cpp
  ${LAUNCHER_SOURCE_DIR}/metaDataPanel.cpp

  ${LAUNCHER_HEADER_DIR}/contentPanel.h
  ${LAUNCHER_HEADER_DIR}/basePanel.h
  ${LAUNCHER_HEADER_DIR}/ezOptionsPanel.h
  ${LAUNCHER_HEADER_DIR}/mainWindow.h
  ${LAUNCHER_HEADER_DIR}/playbackPanel.h
  ${LAUNCHER_HEADER_DIR}/capturePanel.h
  ${LAUNCHER_HEADER_DIR}/subcapturePanel.h
  ${LAUNCHER_HEADER_DIR}/metaDataPanel.h
  )
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

target_include_directories(gits_launcher PRIVATE ${LAUNCHER_HEADER_DIR} 
  ${Vulkan_INCLUDE_DIRS}  
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
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /wd4996  # deprecated warnings
    /wd4244  # conversion warnings
    /wd4267  # size_t conversion warnings
    /wd4018  # signed/unsigned mismatch
  >
)

target_link_libraries(gits_launcher PRIVATE ${Vulkan_LIBRARIES} imgui_common log common)
if(NOT WIN64)
target_link_libraries(gits_launcher PRIVATE glfw)
endif()
add_dependencies(gits_launcher config_codegen generate_font_header)

install(TARGETS gits_launcher
  DESTINATION .
  COMPONENT gits_launcher
)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_launcher>
    DESTINATION .
    COMPONENT gits_launcher
    OPTIONAL
  )
endif()




