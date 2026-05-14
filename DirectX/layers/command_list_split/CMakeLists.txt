# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_command_list_split STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/commandListSplitLayerGroup.h
  ${SRC_DIR}/commandListSplitLayerGroup.cpp
  ${SRC_DIR}/commandListSplitLayerAuto.h
  ${SRC_DIR}/commandListSplitLayerAuto.cpp
  ${SRC_DIR}/commandListSplitLayerCustom.cpp
  ${SRC_DIR}/commandListSplitRecorder.h
  ${SRC_DIR}/commandListSplitRecorder.cpp
  ${SRC_DIR}/commandListSplitService.h
  ${SRC_DIR}/commandListSplitService.cpp
  ${SRC_DIR}/executionSerializationKeyAllocator.h
  ${SRC_DIR}/commandListStateService.h
  ${SRC_DIR}/commandListStateService.cpp
)
source_group("" FILES ${ROOT_SRC})

set(ALL_SRC
  ${ROOT_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_command_list_split PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_command_list_split PRIVATE
  ${ARGS_SOURCE_DIR}
)

target_link_libraries(DirectX_command_list_split PRIVATE
  stream
  DirectX_layer_interface
  DirectX_utils
  DirectX_coders
)

set_target_properties(DirectX_command_list_split PROPERTIES FOLDER DirectX/DirectX_layers)
