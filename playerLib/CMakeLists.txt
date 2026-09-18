# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADER_DIR ${SRC_DIR}/include)

set(player_lib_DEF )
if(MSVC)
  set(player_lib_DEF ${CMAKE_CURRENT_BINARY_DIR}/playerLib.def)
  file(WRITE ${player_lib_DEF}
    "LIBRARY gitsPlayer.dll\n"
    "EXPORTS\n"
    "  Initialize\n"
    "  GetApplicationName\n"
    "  Play\n")
endif()

list(APPEND player_lib_SOURCES
  ${HEADER_DIR}/playerLib.h
  ${SRC_DIR}/playerLib.cpp
  ${SRC_DIR}/streamPlayer.h
  ${SRC_DIR}/streamPlayer.cpp
  ${SRC_DIR}/player.h
  ${SRC_DIR}/player.cpp
  ${SRC_DIR}/statistics.h
  ${SRC_DIR}/statistics.cpp
  ${SRC_DIR}/sequentialExecutor.h
  ${SRC_DIR}/sequentialExecutor.cpp
  ${SRC_DIR}/exceptionHandler.h
  ${SRC_DIR}/playerUtils.h
  ${SRC_DIR}/playerUtils.cpp
  ${player_lib_DEF}
)

add_library(gits_player_lib SHARED ${player_lib_SOURCES})
target_include_directories(gits_player_lib PUBLIC ${HEADER_DIR})

add_library(gits_player_lib_interface INTERFACE)
target_include_directories(gits_player_lib_interface INTERFACE ${HEADER_DIR})

target_include_directories(gits_player_lib PRIVATE
  ${ARGS_SOURCE_DIR}
)

target_link_libraries(gits_player_lib PRIVATE 
  log
  configuration
  common
  stream
  windowing
  OpenGL_common
)

if(WITH_DIRECTX)
#!!! TODO: This should come from linking to DirectX_player
  include_directories(../DirectX/player)
  target_link_libraries(gits_player_lib
    PRIVATE
    d3d12.lib
    dxgi.lib
    DirectX_player
    directxtex
    ${DML_DIR}/bin/x64-win/DirectML.lib
    ${DSTORAGE_DIR}/native/lib/x64/dstorage.lib
    hud
  )
  set_target_properties(gits_player_lib PROPERTIES 
    LINK_FLAGS 
    "/DELAYLOAD:d3d12.dll /DELAYLOAD:dxgi.dll /DELAYLOAD:DirectML.dll /DELAYLOAD:dstorage.dll /DELAYLOAD:dstoragecore.dll"
  )
endif()

if(WITH_LEVELZERO)
  target_link_libraries(gits_player_lib PRIVATE L0_common)
endif()

if(WITH_OPENCL)
  target_link_libraries(gits_player_lib PRIVATE OpenCL_common)
endif()

if(WITH_VULKAN)
  target_link_libraries(gits_player_lib PRIVATE
    Vulkan_player
    VulkanLegacy_common
    hud
  )
endif()

if(WITH_OCLOC)
  target_link_libraries(gits_player_lib PRIVATE ocloc_common)
endif(WITH_OCLOC)

if(MSVC)
  target_link_libraries(gits_player_lib PRIVATE stackwalker)
endif()

if(WIN32)
  set_target_properties(gits_player_lib PROPERTIES OUTPUT_NAME "gitsPlayer")
endif()
if(UNIX)
  set_target_properties(gits_player_lib PROPERTIES OUTPUT_NAME "GitsPlayer")
endif()

install(TARGETS gits_player_lib
  RUNTIME DESTINATION Player
  LIBRARY DESTINATION Player)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:gits_player_lib> DESTINATION Player OPTIONAL)
endif()
