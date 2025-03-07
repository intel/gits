# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_skip_calls STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/skipCallsFactory.h
  ${SRC_DIR}/skipCallsFactory.cpp
)
source_group("" FILES ${ROOT_SRC})

set(SKIP_CALLS_SRC
  ${SRC_DIR}/skipCallsLayerAuto.h
  ${SRC_DIR}/skipCallsLayerAuto.cpp
)
source_group("skip_calls" FILES ${SKIP_CALLS_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${SKIP_CALLS_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_skip_calls PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_skip_calls PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_skip_calls PROPERTIES FOLDER DirectX/DirectX_layers)
