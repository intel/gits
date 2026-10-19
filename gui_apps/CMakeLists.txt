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

# Order is a dependency chain, not a preference: gui_common first because the
# modules link it, then the modules because gui_modules_link() names their
# targets, and the applications last.
#
# No application directory is declared here. A module depends on gui_common and
# on nothing application specific, so it builds identically for the launcher and
# for the studio.
add_subdirectory(gui_common)
add_subdirectory(modules)
add_subdirectory(launcher)
if(WIN64)
add_subdirectory(studio)
endif()