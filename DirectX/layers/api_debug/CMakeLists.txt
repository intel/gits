# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_api_debug STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(DEBUGINFO_SRC
  ${SRC_DIR}/debugInfo.h
  ${SRC_DIR}/debugInfo.cpp
  ${SRC_DIR}/debugInfoLayerAuto.h
  ${SRC_DIR}/debugInfoLayerAuto.cpp
  ${SRC_DIR}/debugHelperLayer.h
  ${SRC_DIR}/debugHelperLayer.cpp
)
source_group("debug_info" FILES ${DEBUGINFO_SRC})

set(LOGDXERROR_SRC
  ${SRC_DIR}/logDxErrorLayerAuto.h
  ${SRC_DIR}/logDxErrorLayerAuto.cpp
)
source_group("log_dx_error" FILES ${LOGDXERROR_SRC})

set(ALL_SRC
  ${DEBUGINFO_SRC}
  ${LOGDXERROR_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_api_debug PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_api_debug PRIVATE
  ${FASTIO_DIR}/include
)

target_link_libraries(DirectX_api_debug PRIVATE
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_api_debug PROPERTIES FOLDER DirectX/DirectX_layers)