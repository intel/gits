# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_address_pinning STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/addressPinningFactory.h
  ${SRC_DIR}/addressPinningFactory.cpp
)
source_group("" FILES ${ROOT_SRC})

set(ADDRESS_PINNING_USE_SRC
  ${SRC_DIR}/addressPinningUseLayer.h
  ${SRC_DIR}/addressPinningUseLayer.cpp
)
source_group("use" FILES ${ADDRESS_PINNING_USE_SRC})

set(ADDRESS_PINNING_STORE_SRC
  ${SRC_DIR}/addressPinningStoreLayer.h
  ${SRC_DIR}/addressPinningStoreLayer.cpp
)
source_group("store" FILES ${ADDRESS_PINNING_STORE_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${ADDRESS_PINNING_USE_SRC}
  ${ADDRESS_PINNING_STORE_SRC}
)

target_sources(DirectX_address_pinning PRIVATE
  ${ALL_SRC}
)

target_link_libraries(DirectX_address_pinning PRIVATE
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_address_pinning PROPERTIES FOLDER DirectX/DirectX_layers)