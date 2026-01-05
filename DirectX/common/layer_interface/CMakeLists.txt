# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_layer_interface STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(ALL_SRC
  ${SRC_DIR}/include/directx.h
  ${SRC_DIR}/include/intelExtensions.h
  ${SRC_DIR}/include/commandIdsAuto.h
  ${SRC_DIR}/include/commandsAuto.h
  ${SRC_DIR}/include/commandsCustom.h
  ${SRC_DIR}/include/layerAuto.h
  ${SRC_DIR}/include/command.h
  ${SRC_DIR}/include/arguments.h
  ${SRC_DIR}/arguments.cpp
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_layer_interface PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_layer_interface PUBLIC
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
  ${NVAPI_DIR}
  ${SRC_DIR}/include
)

add_dependencies(DirectX_layer_interface
  DirectX_codegen
  config_codegen
)

target_link_libraries(DirectX_layer_interface PUBLIC
  common
)

set_target_properties(DirectX_layer_interface PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(DirectX_layer_interface PROPERTIES FOLDER DirectX/DirectX_common)