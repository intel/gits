# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_execution_serialization STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/executionSerializationFactory.h
  ${SRC_DIR}/executionSerializationFactory.cpp
  ${SRC_DIR}/executionSerializationLayerAuto.h
  ${SRC_DIR}/executionSerializationLayerAuto.cpp
  ${SRC_DIR}/executionSerializationLayerCustom.cpp
  ${SRC_DIR}/executionSerializationRecorder.h
  ${SRC_DIR}/executionSerializationRecorder.cpp
  ${SRC_DIR}/directXApiIfaceExecutionSerialization.h
  ${SRC_DIR}/commandListExecutionService.h
  ${SRC_DIR}/commandListExecutionService.cpp
  ${SRC_DIR}/cpuDescriptorsService.h
  ${SRC_DIR}/cpuDescriptorsService.cpp
)
source_group("" FILES ${ROOT_SRC})

set(ALL_SRC
  ${ROOT_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_execution_serialization PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_execution_serialization PRIVATE
  ${DIRECTX_DIR}/common/coders
)

target_link_libraries(DirectX_execution_serialization PRIVATE
  DirectX_layer_interface
  DirectX_dstorage_resources
  DirectX_utils
  DirectX_coders
  common
  configuration
)

set_target_properties(DirectX_execution_serialization PROPERTIES FOLDER DirectX/DirectX_layers)
