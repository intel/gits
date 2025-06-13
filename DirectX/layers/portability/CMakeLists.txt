# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_portability STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ALL_SRC
  ${SRC_DIR}/portabilityFactory.h
  ${SRC_DIR}/portabilityFactory.cpp
  ${SRC_DIR}/portabilityLayer.h
  ${SRC_DIR}/portabilityLayer.cpp
  ${SRC_DIR}/resourcePlacementCapture.h
  ${SRC_DIR}/resourcePlacementCapture.cpp
  ${SRC_DIR}/resourcePlacementPlayback.h
  ${SRC_DIR}/resourcePlacementPlayback.cpp
)
source_group("" FILES ${ALL_SRC})

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_portability PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_portability PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_portability PROPERTIES FOLDER DirectX/DirectX_layers)
