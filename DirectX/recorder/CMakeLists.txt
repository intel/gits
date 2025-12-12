# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_recorder STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/captureManager.h
  ${SRC_DIR}/captureManager.cpp
  ${SRC_DIR}/captureLayerManager.h
  ${SRC_DIR}/captureLayerManager.cpp
)
source_group("" FILES ${ROOT_SRC})

set(INTERCEPTOR_SRC
  ${SRC_DIR}/functionDispatchTables.h
  ${SRC_DIR}/wrappersAuto.cpp
  ${SRC_DIR}/wrappersAuto.h
  ${SRC_DIR}/wrapperCreatorsAuto.h
  ${SRC_DIR}/wrapperCreatorsAuto.cpp
  ${SRC_DIR}/wrapperUtils.h
  ${SRC_DIR}/wrapperUtils.cpp
  ${SRC_DIR}/iunknownWrapper.h
  ${SRC_DIR}/iunknownWrapper.cpp
  ${SRC_DIR}/interceptorCustomizationLayer.h
  ${SRC_DIR}/interceptorCustomizationLayer.cpp
  ${SRC_DIR}/kernelWrappers.h
  ${SRC_DIR}/kernelWrappers.cpp
  ${SRC_DIR}/interfaceArgumentUpdaters.h
  ${SRC_DIR}/interfaceArgumentUpdaters.cpp
  ${SRC_DIR}/intelExtensionsWrappers.h
  ${SRC_DIR}/intelExtensionsWrappers.cpp
  ${SRC_DIR}/nvapiWrappers.h
  ${SRC_DIR}/nvapiWrappers.cpp
  ${SRC_DIR}/nvapiDispatchTable.h
  ${SRC_DIR}/d3d11on12Wrappers.h
  ${SRC_DIR}/d3d11on12Wrappers.cpp
  ${SRC_DIR}/d3d11on12DispatchTable.h
  ${SRC_DIR}/xessDispatchTableAuto.h
)
source_group("interceptor" FILES ${INTERCEPTOR_SRC})

set(CAPTURE_SRC
  ${SRC_DIR}/d3d12RecorderInterface.h
  ${SRC_DIR}/d3d12Recorder.h
  ${SRC_DIR}/d3d12Recorder.cpp
  ${SRC_DIR}/gitsRecorder.h
  ${SRC_DIR}/gitsRecorder.cpp
  ${SRC_DIR}/directXApiIfaceRecorder.h
  ${SRC_DIR}/captureCustomizationLayer.h
  ${SRC_DIR}/captureCustomizationLayer.cpp
  ${SRC_DIR}/captureSynchronizationLayer.h
  ${SRC_DIR}/captureSynchronizationLayer.cpp
  ${SRC_DIR}/encoderLayerAuto.h
  ${SRC_DIR}/encoderLayerAuto.cpp
  ${SRC_DIR}/encoderLayerCustom.cpp
  ${SRC_DIR}/globalSynchronizationLayerAuto.h
  ${SRC_DIR}/globalSynchronizationLayerAuto.cpp
  ${SRC_DIR}/dllOverrideStoreLayer.h
  ${SRC_DIR}/dllOverrideStoreLayer.cpp
)
source_group("capture" FILES ${CAPTURE_SRC})

set(SERVICES_SRC
  ${SRC_DIR}/captureDescriptorHandleService.h
  ${SRC_DIR}/captureDescriptorHandleService.cpp
  ${SRC_DIR}/mapTrackingService.h
  ${SRC_DIR}/mapTrackingService.cpp
  ${SRC_DIR}/rootSignatureService.h
  ${SRC_DIR}/rootSignatureService.cpp
  ${SRC_DIR}/gpuAddressService.h
  ${SRC_DIR}/gpuAddressService.cpp
  ${SRC_DIR}/fenceService.h
  ${SRC_DIR}/fenceService.cpp
  ${SRC_DIR}/contextMapService.h
  ${SRC_DIR}/directStorageService.h
  ${SRC_DIR}/directStorageService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

set(GPU_PATCH_SRC
  ${SRC_DIR}/gpuPatchLayer.h
  ${SRC_DIR}/gpuPatchLayer.cpp
  ${SRC_DIR}/gpuPatchDump.h
  ${SRC_DIR}/gpuPatchDump.cpp
)
source_group("gpu_patch" FILES ${GPU_PATCH_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${INTERCEPTOR_SRC}
  ${CAPTURE_SRC}
  ${SERVICES_SRC}
  ${GPU_PATCH_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_recorder PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_recorder PRIVATE
  ${CMAKE_SOURCE_DIR}/third_party/Detours/src
  ${DIRECTX_DIR}/layers/trace
  ${DIRECTX_DIR}/layers/resource_dumping
  ${DIRECTX_DIR}/layers/portability
  ${DIRECTX_DIR}/layers/api_debug
  ${DIRECTX_DIR}/layers/address_pinning
  ${CMAKE_SOURCE_DIR}/third_party/DirectXTex/DirectXTex
  ${PLUGINS_DIR}
  ${ARGS_SOURCE_DIR}
  ${SRC_DIR}
)

target_link_libraries(DirectX_recorder PRIVATE
  gits_recorder_interface
  DirectX_layer_interface
  DirectX_api_debug
  DirectX_trace
  DirectX_coders
  DirectX_imgui_hud
  DirectX_utils
  DirectX_resource_dumping
  DirectX_portability
  DirectX_address_pinning
  ${INTEL_EXT_DIR}/lib/igdext64.lib 
  ${NVAPI_DIR}/amd64/nvapi64.lib
  tbb
)

set_target_properties(DirectX_recorder PROPERTIES FOLDER DirectX)
