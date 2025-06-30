# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

list(APPEND player_SOURCE
  include/display.h
  include/player.h
  include/sequentialExecutor.h
  include/statistics.h
  include/window.h
  include/playerUtils.h

  player.cpp
  playerMain.cpp
  sequentialExecutor.cpp
  statistics.cpp
  playerUtils.cpp
)

if(MSVC)
  list(APPEND player_SOURCE
    gitsLongPath.manifest)
endif()

add_executable(gitsPlayer ${player_SOURCE})

add_dependencies(gitsPlayer config_codegen)
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

if(WITH_DIRECTX)
  include_directories(../DirectX/player)
  target_link_libraries(gitsPlayer
    d3d12.lib
    dxgi.lib
    DirectX_player
    directxtex
    ${DML_DIR}/bin/x64-win/DirectML.lib
    ${DSTORAGE_DIR}/native/lib/x64/dstorage.lib
    ImGuiHUD
  )
  set_target_properties(gitsPlayer PROPERTIES 
    LINK_FLAGS 
    "/DELAYLOAD:d3d12.dll /DELAYLOAD:dxgi.dll /DELAYLOAD:DirectML.dll /DELAYLOAD:dstorage.dll /DELAYLOAD:dstoragecore.dll"
  )
endif()

target_include_directories(gitsPlayer PRIVATE ${ARGS_SOURCE_DIR})
target_link_libraries(gitsPlayer png zlib lua configuration)

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
