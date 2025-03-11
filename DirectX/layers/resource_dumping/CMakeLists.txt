# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_resource_dumping STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/resourceDumpingFactory.h
  ${SRC_DIR}/resourceDumpingFactory.cpp
)
source_group("" FILES ${ROOT_SRC})

set(SCREENSHOTS_SRC
  ${SRC_DIR}/screenshotsLayer.h
  ${SRC_DIR}/screenshotsLayer.cpp
  ${SRC_DIR}/screenshotDump.h
  ${SRC_DIR}/screenshotDump.cpp
)
source_group("screenshots" FILES ${SCREENSHOTS_SRC})

set(RESOURCE_DUMP_SRC
  ${SRC_DIR}/resourceDumpLayerAuto.h
  ${SRC_DIR}/resourceDumpLayerAuto.cpp
  ${SRC_DIR}/resourceDumpLayerCustom.cpp
  ${SRC_DIR}/resourceDumpService.h
  ${SRC_DIR}/resourceDumpService.cpp
  ${SRC_DIR}/resourceStateTracker.h
  ${SRC_DIR}/resourceStateTracker.cpp
)
source_group("resource_dump" FILES ${RESOURCE_DUMP_SRC})

set(RENDER_TARGETS_DUMP_SRC
  ${SRC_DIR}/renderTargetsDumpLayer.h
  ${SRC_DIR}/renderTargetsDumpLayer.cpp
  ${SRC_DIR}/renderTargetsDump.h
  ${SRC_DIR}/renderTargetsDump.cpp
)
source_group("render_targets_dump" FILES ${RENDER_TARGETS_DUMP_SRC})

set(ACCELERATION_STRUCTURES_DUMP_SRC
  ${SRC_DIR}/accelerationStructuresDumpLayer.h
  ${SRC_DIR}/accelerationStructuresDumpLayer.cpp
  ${SRC_DIR}/accelerationStructuresDump.h
  ${SRC_DIR}/accelerationStructuresDump.cpp
)
source_group("acceleration_structures_dump" FILES ${ACCELERATION_STRUCTURES_DUMP_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${SCREENSHOTS_SRC}
  ${RESOURCE_DUMP_SRC}
  ${RENDER_TARGETS_DUMP_SRC}
  ${ACCELERATION_STRUCTURES_DUMP_SRC}
)

set_auto_generated_property(FILE_LIST  ${ALL_SRC})

target_sources(DirectX_resource_dumping PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_resource_dumping PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/utils/to_string
  ${DIRECTX_DIR}/common/utils
  ${D3D12_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${XESS_HEADERS_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/DirectXTex/DirectXTex
)

set_target_properties(DirectX_resource_dumping PROPERTIES FOLDER DirectX/DirectX_layers)
