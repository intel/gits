# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

list(APPEND player_SOURCE
  include/display.h
  include/player.h
  include/playerOptions.h
  include/sequentialExecutor.h
  include/statistics.h
  include/window.h

  player.cpp
  playerMain.cpp
  playerOptions.cpp
  sequentialExecutor.cpp
  statistics.cpp
)

if(MSVC)
  list(APPEND player_SOURCE
    gitsLongPath.manifest)
endif()

add_executable(gitsPlayer ${player_SOURCE})

target_link_libraries(gitsPlayer common OpenGL_common)

if(WITH_LEVELZERO)
  target_link_libraries(gitsPlayer L0_common)
endif()

if(WITH_OPENCL)
  target_link_libraries(gitsPlayer OpenCL_common)
endif()

if(WITH_VULKAN)
  target_link_libraries(gitsPlayer Vulkan_common)
endif()

if(WITH_OCLOC)
  target_link_libraries(gitsPlayer ocloc_common)
endif(WITH_OCLOC)

target_link_libraries(gitsPlayer png zlib lua)

if(MSVC)
  target_link_libraries(gitsPlayer stackwalker)
endif()

if(UNIX)
  target_link_libraries(gitsPlayer pthread)
  target_link_libraries(gitsPlayer GL X11 xcb X11-xcb dl)
endif()

if(MSVC)
  target_link_libraries(gitsPlayer OpenGL32)
endif()

install(TARGETS gitsPlayer DESTINATION Player)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gitsPlayer> DESTINATION Player OPTIONAL)
endif()
