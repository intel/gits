# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_ccode STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ALL_SRC
  ${SRC_DIR}/ccodeParameters.h
  ${SRC_DIR}/ccodeStringLiterals.h
  ${SRC_DIR}/ccodeStream.h
  ${SRC_DIR}/ccodeStream.cpp
  ${SRC_DIR}/ccodeLayerAuto.h
  ${SRC_DIR}/ccodeLayerAuto.cpp
  ${SRC_DIR}/ccodeLayerCustom.cpp
  ${SRC_DIR}/ccodeCommandPrinter.h
  ${SRC_DIR}/ccodeCommandPrinter.cpp
  ${SRC_DIR}/ccodeArguments.h
  ${SRC_DIR}/ccodeArguments.cpp
  ${SRC_DIR}/ccodeTypes.h
  ${SRC_DIR}/ccodeTypes.cpp
  ${SRC_DIR}/ccodeStructsAuto.h
  ${SRC_DIR}/ccodeStructsAuto.cpp
  ${SRC_DIR}/ccodeStructsCustom.cpp
  ${SRC_DIR}/ccodeUtilsAuto.h
  ${SRC_DIR}/ccodeUtilsAuto.cpp
)
source_group("" FILES ${ROOT_SRC})

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_ccode PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_ccode
PUBLIC
  ${SRC_DIR}
)

target_link_libraries(DirectX_ccode PRIVATE
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_ccode PROPERTIES FOLDER DirectX/DirectX_layers)
