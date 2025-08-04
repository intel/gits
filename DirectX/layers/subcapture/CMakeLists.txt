# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_subcapture STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/subcaptureFactory.h
  ${SRC_DIR}/subcaptureFactory.cpp
)
source_group("" FILES ${ROOT_SRC})

set(RECORDING_SRC
  ${SRC_DIR}/subcaptureRecorder.h
  ${SRC_DIR}/subcaptureRecorder.cpp
  ${SRC_DIR}/directXApiIfaceSubcapture.h
  ${SRC_DIR}/recordingLayerAuto.h
  ${SRC_DIR}/recordingLayerAuto.cpp
  ${SRC_DIR}/recordingLayerCustom.cpp
  ${SRC_DIR}/commandPreservationLayer.h
  ${SRC_DIR}/commandPreservationLayer.cpp
  ${SRC_DIR}/subcaptureRange.h
  ${SRC_DIR}/subcaptureRange.cpp
)
source_group("recording" FILES ${RECORDING_SRC})

set(STATE_TRACKING_SRC
  ${SRC_DIR}/stateTrackingLayer.h
  ${SRC_DIR}/stateTrackingLayer.cpp
  ${SRC_DIR}/stateTrackingService.h
  ${SRC_DIR}/stateTrackingService.cpp
  ${SRC_DIR}/objectState.h
  ${SRC_DIR}/resourceContentRestore.h
  ${SRC_DIR}/resourceContentRestore.cpp
  ${SRC_DIR}/fenceTrackingService.h
  ${SRC_DIR}/mapStateService.h
  ${SRC_DIR}/mapStateService.cpp
  ${SRC_DIR}/resourceStateTrackingService.h
  ${SRC_DIR}/resourceStateTrackingService.cpp
  ${SRC_DIR}/heapAllocationStateService.h
  ${SRC_DIR}/reservedResourcesService.h
  ${SRC_DIR}/reservedResourcesService.cpp
  ${SRC_DIR}/descriptorService.h
  ${SRC_DIR}/descriptorService.cpp
  ${SRC_DIR}/commandListService.h
  ${SRC_DIR}/commandListService.cpp
  ${SRC_DIR}/commandQueueService.h
  ${SRC_DIR}/commandQueueService.cpp
  ${SRC_DIR}/xessStateService.h
  ${SRC_DIR}/xessStateService.cpp
  ${SRC_DIR}/accelerationStructuresBuildService.h
  ${SRC_DIR}/accelerationStructuresBuildService.cpp
  ${SRC_DIR}/accelerationStructuresSerializeService.h
  ${SRC_DIR}/accelerationStructuresSerializeService.cpp
  ${SRC_DIR}/accelerationStructuresBufferContentRestore.h
  ${SRC_DIR}/accelerationStructuresBufferContentRestore.cpp
  ${SRC_DIR}/gpuExecutionFlusher.h
  ${SRC_DIR}/gpuExecutionFlusher.cpp
  ${SRC_DIR}/residencyService.h
  ${SRC_DIR}/residencyService.cpp
  ${SRC_DIR}/resourceUsageTrackingService.h
  ${SRC_DIR}/resourceUsageTrackingService.cpp
)
source_group("state_tracking" FILES ${STATE_TRACKING_SRC})

set(ANALYSIS_SRC
  ${SRC_DIR}/analyzerLayerAuto.h
  ${SRC_DIR}/analyzerLayerAuto.cpp
  ${SRC_DIR}/analyzerLayerCustom.cpp
  ${SRC_DIR}/analyzerService.h
  ${SRC_DIR}/analyzerService.cpp
  ${SRC_DIR}/analyzerResults.h
  ${SRC_DIR}/analyzerResults.cpp
  ${SRC_DIR}/bindingService.h
  ${SRC_DIR}/bindingService.cpp
  ${SRC_DIR}/rootSignatureService.h
  ${SRC_DIR}/rootSignatureService.cpp
  ${SRC_DIR}/analyzerRaytracingService.h
  ${SRC_DIR}/analyzerRaytracingService.cpp
  ${SRC_DIR}/raytracingInstancesDump.h
  ${SRC_DIR}/raytracingInstancesDump.cpp
  ${SRC_DIR}/bindingTablesDump.h
  ${SRC_DIR}/bindingTablesDump.cpp
)
source_group("analysis" FILES ${ANALYSIS_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${RECORDING_SRC}
  ${STATE_TRACKING_SRC}
  ${ANALYSIS_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_subcapture PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_subcapture PRIVATE
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/coders
  ${DIRECTX_DIR}/common/utils
  ${DIRECTX_DIR}/layers/dstorage_resources
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${XESS_HEADERS_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/DirectXTex/DirectXTex
)

set_target_properties(DirectX_subcapture PROPERTIES FOLDER DirectX/DirectX_layers)
