# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(PLAYER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND player_SOURCE
  ${PLAYER_SOURCE_DIR}/playerMain.cpp
)

if(MSVC)
  list(APPEND player_SOURCE
    gitsLongPath.manifest)
endif()

add_executable(gits_player ${player_SOURCE})
set_target_properties(gits_player PROPERTIES OUTPUT_NAME "gitsPlayer")

target_link_libraries(gits_player PRIVATE configuration stream gits_player_lib)

if(UNIX)
  target_link_libraries(gits_player PRIVATE pthread)
  set_target_properties(gits_player PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN")
endif()

# Plog shared instance
if(WIN32)
    target_compile_definitions(gits_player PRIVATE PLOG_EXPORT)
else()
    target_compile_definitions(gits_player PRIVATE PLOG_GLOBAL)
endif()

install(TARGETS gits_player DESTINATION Player)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_player> DESTINATION Player OPTIONAL)
endif()
