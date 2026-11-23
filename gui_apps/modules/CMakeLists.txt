# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Makes a SHARED library target a Studio module, loaded from Modules/<folder>/module.dll. Name the
# folder after the module's id, like a GITS plugin folder. Links what every module needs. With
# INSTALL, the DLL and its PDB ship in the gits_studio component.
function(gui_module target folder)
  cmake_parse_arguments(ARG "INSTALL" "" "" ${ARGN})
  if(NOT WIN64)
    message(FATAL_ERROR "Studio modules are Windows x64 DLLs: ${target}")
  endif()
  # gui_common and imgui_common rely on the executable for the ImGui add-ons; a module DLL links
  # its own copy.
  target_link_libraries(${target} PRIVATE
    imgui_addons module_common gui_common common_interface imgui_common log configuration stream)
  add_dependencies(${target} config_codegen)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME module FOLDER "${GUI_MODULES_FOLDER}")
  # The build-tree studio runs with every module built in this tree. The copy belongs to the
  # module's build, not to the studio's, so a rebuilt module reaches the studio even when the
  # studio itself is up to date.
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GUI_STUDIO_RUNTIME_DIR}/Modules/${folder}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:${target}>" "${GUI_STUDIO_RUNTIME_DIR}/Modules/${folder}"
  )
  set_property(GLOBAL APPEND PROPERTY GUI_MODULE_TARGETS ${target})
  if(ARG_INSTALL)
    install(TARGETS ${target} RUNTIME DESTINATION "Modules/${folder}" COMPONENT gits_studio)
    if(MSVC)
      install(FILES "$<TARGET_PDB_FILE:${target}>"
        DESTINATION "Modules/${folder}" COMPONENT gits_studio OPTIONAL)
    endif()
  endif()
endfunction()

# Each module folder guards its own target with if(WIN64).
add_subdirectory(common)
add_subdirectory(rtsr)
add_subdirectory(hello_module)
