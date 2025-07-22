# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_utils STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(UTILS_FILES
  ${SRC_DIR}/pluginService.h
  ${SRC_DIR}/pluginService.cpp
  ${SRC_DIR}/resourceDump.h
  ${SRC_DIR}/resourceDump.cpp
  ${SRC_DIR}/gpuExecutionTracker.h
  ${SRC_DIR}/gpuExecutionTracker.cpp
  ${SRC_DIR}/configKeySet.h
  ${SRC_DIR}/configKeySet.cpp
  ${SRC_DIR}/capturePlayerGpuAddressService.h
  ${SRC_DIR}/capturePlayerGpuAddressService.cpp
)
source_group("" FILES ${UTILS_FILES})

set(TO_STR_FILES
  ${SRC_DIR}/to_string/toStr.h
  ${SRC_DIR}/to_string/toStr.cpp
  ${SRC_DIR}/to_string/enumToStrAuto.h
  ${SRC_DIR}/to_string/enumToStrAuto.cpp
  ${SRC_DIR}/to_string/guidToStrAuto.h
  ${SRC_DIR}/to_string/guidToStrAuto.cpp
)
source_group("to_string" FILES ${TO_STR_FILES})

set(ALL_SRC
  ${UTILS_FILES}
  ${TO_STR_FILES}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_utils PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_utils PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${XESS_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${XESS_HEADERS_DIR}
  ${PLUGINS_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/DirectXTex/DirectXTex
)

set_target_properties(DirectX_utils PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(DirectX_utils PROPERTIES FOLDER DirectX/DirectX_common)
