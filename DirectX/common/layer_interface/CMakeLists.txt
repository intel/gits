# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_layer_interface STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LAYER_INTERFACE_SRC
  ${SRC_DIR}/directx.h
  ${SRC_DIR}/intelExtensions.h
  ${SRC_DIR}/commandIdsAuto.h
  ${SRC_DIR}/commandsAuto.h
  ${SRC_DIR}/commandsCustom.h
  ${SRC_DIR}/layerAuto.h
  ${SRC_DIR}/command.h
  ${SRC_DIR}/arguments.h
  ${SRC_DIR}/arguments.cpp
)
source_group("" FILES ${LAYER_INTERFACE_SRC})
set_auto_generated_property(FILE_LIST ${LAYER_INTERFACE_SRC})

target_sources(DirectX_layer_interface PRIVATE
  ${LAYER_INTERFACE_SRC}
)

target_include_directories(DirectX_layer_interface PRIVATE
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_layer_interface PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(DirectX_layer_interface PROPERTIES FOLDER DirectX/DirectX_common)