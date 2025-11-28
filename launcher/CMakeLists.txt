set(LAUNCHER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LAUNCHER_HEADER_DIR ${LAUNCHER_SOURCE_DIR}/include)

find_package(Vulkan REQUIRED)

list(APPEND LAUNCHER_SOURCE
  ${LAUNCHER_SOURCE_DIR}/fileActions.cpp
  ${LAUNCHER_SOURCE_DIR}/main.cpp
  ${LAUNCHER_SOURCE_DIR}/mainWindow.cpp
  ${LAUNCHER_SOURCE_DIR}/mainWindowDesign.cpp


  ${LAUNCHER_HEADER_DIR}/launcherConfig.h
  ${LAUNCHER_HEADER_DIR}/fileActions.h
  ${LAUNCHER_HEADER_DIR}/mainWindow.h
  ${LAUNCHER_HEADER_DIR}/context.h
  ${LAUNCHER_HEADER_DIR}/resource.h
)

list(APPEND IMGUI_BACKEND_SOURCE
  ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
  ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
)
source_group("ImGui-Backends" FILES ${IMGUI_BACKEND_SRC})

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

add_executable(gits_launcher ${LAUNCHER_SOURCE} ${IMGUI_BACKEND_SOURCE} ${IMGUI_ADDONS_SOURCE} ${LAUNCHER_SOURCE_DIR}/gitsLauncher.rc)
set_target_properties(gits_launcher PROPERTIES OUTPUT_NAME "gitsLauncher")

target_include_directories(gits_launcher PRIVATE ${LAUNCHER_HEADER_DIR} ${Vulkan_INCLUDE_DIRS} ${IMGUI_BACKEND_DIR} ${IMGUI_COLOR_TEXT_EDIT_DIR} ${IMGUI_FILE_DIALOG_DIR} ${PLOG_SOURCE_DIR}/include ${COMMON_HEADER_DIR})

target_link_libraries(gits_launcher PRIVATE ${Vulkan_LIBRARIES} imgui_common log)
add_dependencies(gits_launcher config_codegen generate_font_header)

install(TARGETS gits_launcher DESTINATION Launcher)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_launcher> DESTINATION Launcher OPTIONAL)
endif()




