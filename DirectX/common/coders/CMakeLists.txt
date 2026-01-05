# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_coders STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(COMMON_SRC
  ${SRC_DIR}/include/dmlCodersCustom.h
  ${SRC_DIR}/include/dmlCodersAuto.h
  ${SRC_DIR}/dmlCodersCustom.cpp
  ${SRC_DIR}/dmlCodersAuto.cpp
)
source_group("common" FILES ${COMMON_SRC})

set(ENCODERS_SRC
  ${SRC_DIR}/include/argumentEncoders.h
  ${SRC_DIR}/include/commandWriter.h
  ${SRC_DIR}/include/commandWritersFactory.h
  ${SRC_DIR}/include/commandWritersCustom.h
  ${SRC_DIR}/include/commandEncodersCustom.h
  ${SRC_DIR}/include/commandWritersAuto.h
  ${SRC_DIR}/include/commandEncodersAuto.h
  ${SRC_DIR}/argumentEncoders.cpp
  ${SRC_DIR}/commandEncodersAuto.cpp
  ${SRC_DIR}/commandEncodersCustom.cpp
  ${SRC_DIR}/commandWritersFactoryAuto.cpp
)
source_group("encoders" FILES ${ENCODERS_SRC})

set(DECODERS_SRC
  ${SRC_DIR}/include/argumentDecoders.h
  ${SRC_DIR}/include/commandDecodersAuto.h
  ${SRC_DIR}/include/commandDecodersCustom.h
  ${SRC_DIR}/argumentDecoders.cpp
  ${SRC_DIR}/commandDecodersCustom.cpp
  ${SRC_DIR}/commandDecodersAuto.cpp
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

target_include_directories(DirectX_coders PUBLIC
  ${SRC_DIR}/include
)

target_link_libraries(DirectX_coders PRIVATE
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_coders PROPERTIES FOLDER DirectX/DirectX_common)