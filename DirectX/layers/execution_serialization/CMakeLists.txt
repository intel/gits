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
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/coders
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_execution_serialization PROPERTIES FOLDER DirectX/DirectX_layers)
