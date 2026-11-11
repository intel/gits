# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Makes a SHARED library target the Studio module <id>. Studio loads Modules/<id>/module.dll and
# calls the GitsGuiModule export of GITS_GUI_REGISTER_MODULE.
function(gui_module target id)
  if(NOT WIN64)
    message(FATAL_ERROR "Studio modules are Windows x64 DLLs: ${target}")
  endif()
  # gui_common and imgui_common rely on the executable for the ImGui add-ons; a module DLL links
  # its own copy.
  target_link_libraries(${target} PRIVATE imgui_addons)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME module GUI_MODULE_ID ${id})
  # The build-tree studio runs with every module built in this tree. The copy belongs to the
  # module's build, not to the studio's, so a rebuilt module reaches the studio even when the
  # studio itself is up to date.
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GUI_STUDIO_RUNTIME_DIR}/Modules/${id}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:${target}>" "${GUI_STUDIO_RUNTIME_DIR}/Modules/${id}"
  )
  set_property(GLOBAL APPEND PROPERTY GUI_MODULE_TARGETS ${target})
endfunction()

# Installs a module DLL where Studio looks for it: Modules/<id>, next to Studio or in the GITS
# installation.
function(gui_module_install target)
  get_target_property(id ${target} GUI_MODULE_ID)
  install(TARGETS ${target} RUNTIME DESTINATION "Modules/${id}" COMPONENT gits_studio)
  if(MSVC)
    install(FILES "$<TARGET_PDB_FILE:${target}>"
      DESTINATION "Modules/${id}" COMPONENT gits_studio OPTIONAL)
  endif()
endfunction()

add_subdirectory(common)
add_subdirectory(rtsr)
if(WIN64)
  add_subdirectory(hello_module)
endif()
