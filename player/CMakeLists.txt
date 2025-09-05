# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(PLAYER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLAYER_HEADER_DIR ${PLAYER_SOURCE_DIR}/include)

list(APPEND player_SOURCE
  ${PLAYER_HEADER_DIR}/display.h
  ${PLAYER_HEADER_DIR}/player.h
  ${PLAYER_HEADER_DIR}/sequentialExecutor.h
  ${PLAYER_HEADER_DIR}/statistics.h
  ${PLAYER_HEADER_DIR}/window.h
  ${PLAYER_HEADER_DIR}/playerUtils.h

  ${PLAYER_SOURCE_DIR}/player.cpp
  ${PLAYER_SOURCE_DIR}/playerMain.cpp
  ${PLAYER_SOURCE_DIR}/sequentialExecutor.cpp
  ${PLAYER_SOURCE_DIR}/statistics.cpp
  ${PLAYER_SOURCE_DIR}/playerUtils.cpp
)

if(MSVC)
  list(APPEND player_SOURCE
    gitsLongPath.manifest)
endif()

add_executable(gits_player ${player_SOURCE})
set_target_properties(gits_player PROPERTIES OUTPUT_NAME "gitsPlayer")

target_include_directories(gits_player PRIVATE ${PLAYER_HEADER_DIR})

add_dependencies(gits_player config_codegen)
target_link_libraries(gits_player PRIVATE common OpenGL_common)

if(WITH_LEVELZERO)
  target_link_libraries(gits_player PRIVATE L0_common)
endif()

if(WITH_OPENCL)
  target_link_libraries(gits_player PRIVATE OpenCL_common)
endif()

if(WITH_VULKAN)
  target_link_libraries(gits_player PRIVATE Vulkan_common)
endif()

if(WITH_OCLOC)
  target_link_libraries(gits_player PRIVATE ocloc_common)
endif(WITH_OCLOC)

if(WITH_DIRECTX)
#!!! TODO: This should come from linking to DirectX_player
  include_directories(../DirectX/player)
  target_link_libraries(gits_player
    PRIVATE
    d3d12.lib
    dxgi.lib
    DirectX_player
    directxtex
    ${DML_DIR}/bin/x64-win/DirectML.lib
    ${DSTORAGE_DIR}/native/lib/x64/dstorage.lib
    hud
  )
  set_target_properties(gits_player PROPERTIES 
    LINK_FLAGS 
    "/DELAYLOAD:d3d12.dll /DELAYLOAD:dxgi.dll /DELAYLOAD:DirectML.dll /DELAYLOAD:dstorage.dll /DELAYLOAD:dstoragecore.dll"
  )
endif()

target_include_directories(gits_player PRIVATE ${ARGS_SOURCE_DIR})
target_link_libraries(gits_player PRIVATE configuration)

if(MSVC)
  target_link_libraries(gits_player PRIVATE stackwalker)
endif()

if(UNIX)
  target_link_libraries(gits_player PRIVATE pthread)
  target_link_libraries(gits_player PRIVATE GL X11 xcb X11-xcb dl)
endif()

if(MSVC)
  target_link_libraries(gits_player PRIVATE OpenGL32)
endif()

install(TARGETS gits_player DESTINATION Player)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_player> DESTINATION Player OPTIONAL)
endif()
