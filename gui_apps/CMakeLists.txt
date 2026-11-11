# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GUI_APPS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GUI_MODULES_DIR ${GUI_APPS_DIR}/modules)
set(GUI_MODULES_FOLDER "${GUI_APPS_FOLDER}/modules")
# Where the build-tree studio runs from. gui_module() copies every module built in
# this tree into its Modules folder, so the studio runs with all of them.
set(GUI_STUDIO_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/studio/$<CONFIG>")

# Order is a dependency chain, not a preference: gui_common first because the
# modules link it, then the modules because the studio collects their targets,
# and the applications last.
#
# No application directory is declared here. A module depends on gui_common and
# on nothing application specific.
add_subdirectory(gui_common)
add_subdirectory(modules)
add_subdirectory(launcher)
if(WIN64)
add_subdirectory(studio)
endif()