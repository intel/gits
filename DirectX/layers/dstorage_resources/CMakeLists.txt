# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_dstorage_resources STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(DSTORAGE_LAYER_SRC
  ${SRC_DIR}/directStorageResourcesLayer.h
  ${SRC_DIR}/directStorageResourcesLayer.cpp
)
source_group("" FILES ${DSTORAGE_LAYER_SRC})

set(ALL_SRC
  ${DSTORAGE_LAYER_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_dstorage_resources PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_dstorage_resources PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_dstorage_resources PROPERTIES FOLDER DirectX/DirectX_layers)
