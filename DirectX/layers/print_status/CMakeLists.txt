# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_print_status STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ALL_SRC
  ${SRC_DIR}/printStatusLayer.h
  ${SRC_DIR}/printStatusLayer.cpp
)
source_group("" FILES ${ROOT_SRC})

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_print_status PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_print_status PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/utils/to_string
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
)

target_link_libraries(DirectX_print_status PRIVATE common configuration)

set_target_properties(DirectX_print_status PROPERTIES FOLDER DirectX/DirectX_layers)
