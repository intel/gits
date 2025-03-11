# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_library(DirectX_trace STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(TRACE_SRC
  ${SRC_DIR}/commandPrinter.h
  ${SRC_DIR}/commandPrinter.cpp
  ${SRC_DIR}/traceLayerCustom.cpp
  ${SRC_DIR}/showExecutionLayer.h
  ${SRC_DIR}/showExecutionLayer.cpp
  ${SRC_DIR}/traceLayerAuto.h
  ${SRC_DIR}/traceLayerAuto.cpp
)
source_group("trace" FILES ${TRACE_SRC})

set(PRINT_SRC
  ${SRC_DIR}/printStructuresAuto.h
  ${SRC_DIR}/printStructuresAuto.cpp
  ${SRC_DIR}/printEnumsAuto.h
  ${SRC_DIR}/printEnumsAuto.cpp
  ${SRC_DIR}/printCustom.h
  ${SRC_DIR}/printCustom.cpp
  ${SRC_DIR}/printArguments.h
  ${SRC_DIR}/printArguments.cpp
)
source_group("print" FILES ${PRINT_SRC})

set(ROOT_SRC
  ${SRC_DIR}/traceFactory.h
  ${SRC_DIR}/traceFactory.cpp
  ${SRC_DIR}/fastOStream.h
  ${SRC_DIR}/fastOStream.cpp
)
source_group("" FILES ${ROOT_SRC})

set(ALL_SRC
  ${TRACE_SRC}
  ${PRINT_SRC}
  ${ROOT_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_trace PRIVATE
  ${ALL_SRC}
)

add_subdirectory(ipc)

target_include_directories(DirectX_trace PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/utils/to_string
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${XESS_HEADERS_DIR}
  ${FASTIO_DIR}/include
)

set_target_properties(DirectX_trace PROPERTIES FOLDER DirectX/DirectX_layers)
