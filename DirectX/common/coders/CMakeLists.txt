# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_coders STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(COMMON_SRC
  ${SRC_DIR}/dmlCodersCustom.h
  ${SRC_DIR}/dmlCodersCustom.cpp
  ${SRC_DIR}/dmlCodersAuto.h
  ${SRC_DIR}/dmlCodersAuto.cpp
)
source_group("common" FILES ${COMMON_SRC})

set(ENCODERS_SRC
  ${SRC_DIR}/commandWriter.h
  ${SRC_DIR}/commandWritersAuto.h
  ${SRC_DIR}/commandEncodersAuto.h
  ${SRC_DIR}/commandEncodersAuto.cpp
  ${SRC_DIR}/argumentEncoders.h
  ${SRC_DIR}/argumentEncoders.cpp
  ${SRC_DIR}/commandWritersCustom.h
  ${SRC_DIR}/commandEncodersCustom.h
  ${SRC_DIR}/commandEncodersCustom.cpp
)
source_group("encoders" FILES ${ENCODERS_SRC})

set(DECODERS_SRC
  ${SRC_DIR}/argumentDecoders.h
  ${SRC_DIR}/argumentDecoders.cpp
  ${SRC_DIR}/commandDecodersAuto.h
  ${SRC_DIR}/commandDecodersAuto.cpp
  ${SRC_DIR}/commandDecodersCustom.h
  ${SRC_DIR}/commandDecodersCustom.cpp
)
source_group("decoders" FILES ${DECODERS_SRC})

set(ALL_SRC
  ${COMMON_SRC}
  ${ENCODERS_SRC}
  ${DECODERS_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_coders PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_coders PRIVATE
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
  ../layer_interface
)

set_target_properties(DirectX_coders PROPERTIES FOLDER DirectX/DirectX_common)