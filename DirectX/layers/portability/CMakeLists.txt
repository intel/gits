# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_portability STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ALL_SRC
  ${SRC_DIR}/portabilityFactory.h
  ${SRC_DIR}/portabilityFactory.cpp
  ${SRC_DIR}/portabilityLayer.h
  ${SRC_DIR}/portabilityLayer.cpp
  ${SRC_DIR}/resourcePlacementCapture.h
  ${SRC_DIR}/resourcePlacementCapture.cpp
  ${SRC_DIR}/resourcePlacementCaptureNoExecute.h
  ${SRC_DIR}/resourcePlacementCaptureNoExecute.cpp
  ${SRC_DIR}/resourcePlacementPlayback.h
  ${SRC_DIR}/resourcePlacementPlayback.cpp
  ${SRC_DIR}/resourcePlacementAssertions.h
  ${SRC_DIR}/resourcePlacementAssertions.cpp
)
source_group("" FILES ${ALL_SRC})

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_portability PRIVATE
  ${ALL_SRC}
)

target_link_libraries(DirectX_portability PRIVATE
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_portability PROPERTIES FOLDER DirectX/DirectX_layers)
