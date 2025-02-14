# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_portability STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(UTILS_SRC
  ${SRC_DIR}/asyncTaskScheduler.h
  ${SRC_DIR}/heapPlacementOrganizer.cpp
  ${SRC_DIR}/heapPlacementOrganizer.h
)
source_group("utils" FILES ${UTILS_SRC})

set(PORTABILITY_LAYER_SRC
  ${SRC_DIR}/portabilityLayer.h
  ${SRC_DIR}/portabilityLayer.cpp
)
source_group("" FILES ${PORTABILITY_LAYER_SRC})

set(ALL_SRC
  ${UTILS_SRC}
  ${PORTABILITY_LAYER_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_portability PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_portability PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${SRC_DIR}/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

set_target_properties(DirectX_portability PROPERTIES FOLDER DirectX/DirectX_layers)
