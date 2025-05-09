# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_player STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/playerManager.h
  ${SRC_DIR}/playerManager.cpp
)
source_group("" FILES ${ROOT_SRC})

set(PLAYER_SRC
  ${SRC_DIR}/replayCustomizationLayer.h
  ${SRC_DIR}/replayCustomizationLayer.cpp
  ${SRC_DIR}/multithreadedObjectCreationLayer.h
  ${SRC_DIR}/multithreadedObjectCreationLayer.cpp
  ${SRC_DIR}/multithreadedObjectAwaitLayer.h
  ${SRC_DIR}/multithreadedObjectAwaitLayer.cpp
  ${SRC_DIR}/interfaceArgumentUpdaters.h
  ${SRC_DIR}/interfaceArgumentUpdaters.cpp
  ${SRC_DIR}/intelExtensions.h
)
source_group("player" FILES ${PLAYER_SRC})

set(SERVICES_SRC
  ${SRC_DIR}/windowService.h
  ${SRC_DIR}/windowService.cpp
  ${SRC_DIR}/gpuAddressService.h
  ${SRC_DIR}/gpuAddressService.cpp
  ${SRC_DIR}/replayDescriptorHandleService.h
  ${SRC_DIR}/replayDescriptorHandleService.cpp
  ${SRC_DIR}/mapTrackingService.h
  ${SRC_DIR}/mapTrackingService.cpp
  ${SRC_DIR}/heapAllocationService.h
  ${SRC_DIR}/heapAllocationService.cpp
  ${SRC_DIR}/adapterService.h
  ${SRC_DIR}/intelExtensionsService.h
  ${SRC_DIR}/intelExtensionsService.cpp  
  ${SRC_DIR}/pipelineLibraryService.h
  ${SRC_DIR}/pipelineLibraryService.cpp
  ${SRC_DIR}/xessDispatchTableAuto.h
  ${SRC_DIR}/xessService.h
  ${SRC_DIR}/xessService.cpp
  ${SRC_DIR}/multithreadedObjectCreationService.h
  ${SRC_DIR}/multithreadedObjectCreationService.cpp
  ${SRC_DIR}/contextMapService.h
)
source_group("services" FILES ${SERVICES_SRC})

set(RUNNER_SRC
  ${SRC_DIR}/commandPlayersCustom.h
  ${SRC_DIR}/commandPlayersCustom.cpp
  ${SRC_DIR}/commandPlayer.h
  ${SRC_DIR}/commandPlayersAuto.h
  ${SRC_DIR}/commandPlayersAuto.cpp
  ${SRC_DIR}/directXLibrary.h
  ${SRC_DIR}/directXLibraryAuto.cpp
)
source_group("runner" FILES ${RUNNER_SRC})

set(GPU_PATCH_SRC
  ${SRC_DIR}/gpuPatchLayer.h
  ${SRC_DIR}/gpuPatchLayer.cpp
  ${SRC_DIR}/gpuPatchAddressService.h
  ${SRC_DIR}/gpuPatchAddressService.cpp
  ${SRC_DIR}/raytracingShaderPatchService.h
  ${SRC_DIR}/raytracingShaderPatchService.cpp
  ${SRC_DIR}/shaderIdentifierService.h
  ${SRC_DIR}/shaderIdentifierService.cpp
  ${SRC_DIR}/gpuPatchDescriptorHandleService.h
  ${SRC_DIR}/gpuPatchDescriptorHandleService.cpp
  ${SRC_DIR}/gpuPatchCommandListService.h
  ${SRC_DIR}/gpuPatchCommandListService.cpp
  ${SRC_DIR}/raytracingResourceDump.h
  ${SRC_DIR}/raytracingResourceDump.cpp
  ${SRC_DIR}/gpuPatchDumpService.h
  ${SRC_DIR}/gpuPatchDumpService.cpp
  ${SRC_DIR}/executeIndirectDump.h
  ${SRC_DIR}/executeIndirectDump.cpp
  ${SRC_DIR}/executeIndirectShaderPatchService.h
  ${SRC_DIR}/executeIndirectShaderPatchService.cpp
)
source_group("gpu_patch" FILES ${GPU_PATCH_SRC})

set(DSTORAGE_SRC
  ${SRC_DIR}/dstorage/directStorageLayer.h
  ${SRC_DIR}/dstorage/directStorageLayer.cpp
)
source_group("dstorage" FILES ${DSTORAGE_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${PLAYER_SRC}
  ${SERVICES_SRC}
  ${RUNNER_SRC}
  ${GPU_PATCH_SRC}
  ${DSTORAGE_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(DirectX_player PRIVATE
  ${ALL_SRC}
)

target_include_directories(DirectX_player PRIVATE
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${DXC_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
  ${INTEL_EXT_DIR}/include
  ${DIRECTX_DIR}/common/layer_interface
  ${DIRECTX_DIR}/common/coders
  ${DIRECTX_DIR}/common/utils
  ${DIRECTX_DIR}/layers/trace
  ${DIRECTX_DIR}/layers/subcapture
  ${DIRECTX_DIR}/layers/execution_serialization
  ${DIRECTX_DIR}/layers/api_debug
  ${DIRECTX_DIR}/layers/resource_dumping
  ${DIRECTX_DIR}/layers/skip_calls
  ${DIRECTX_DIR}/layers/portability
  ${DIRECTX_DIR}/recorder
  ${CMAKE_SOURCE_DIR}/third_party/DirectXTex/DirectXTex
  ${PLUGINS_DIR}
  ${XESS_HEADERS_DIR}
  ${SRC_DIR}
)

target_link_libraries(DirectX_player PRIVATE ${INTEL_EXT_DIR}/lib/igdext64.lib)

set_target_properties(DirectX_player PROPERTIES FOLDER DirectX)

set(D3D12_RUNTIME_FILES
  ${AGILITY_SDK_DIR}/bin/x64/D3D12Core.dll
  ${AGILITY_SDK_DIR}/bin/x64/d3d12SDKLayers.dll
  ${DXC_DIR}/bin/x64/dxcompiler.dll
  ${DXC_DIR}/bin/x64/dxil.dll
  ${XESS_SDK_DIR}/bin/libxess.dll
  ${DML_DIR}/bin/x64-win/DirectML.dll
  ${DML_DIR}/bin/x64-win/DirectML.Debug.dll
  ${DSTORAGE_DIR}/native/bin/x64/dstorage.dll
  ${DSTORAGE_DIR}/native/bin/x64/dstoragecore.dll
)

install(FILES
  ${D3D12_RUNTIME_FILES} DESTINATION Player/D3D12
)

# Install and rename License files
install(FILES ${AGILITY_SDK_DIR}/LICENSE.txt DESTINATION Player/D3D12 RENAME LICENSE_AgilitySDK.txt)
install(FILES ${DXC_DIR}/LICENSE-MS.txt DESTINATION Player/D3D12 RENAME LICENSE_DXC_dxil.txt)
install(FILES ${DXC_DIR}/LICENSE-LLVM.txt DESTINATION Player/D3D12 RENAME LICENSE_DXC_dxcompiler.txt)
install(FILES ${XESS_SDK_DIR}/LICENSE.txt DESTINATION Player/D3D12 RENAME LICENSE_XESS.txt)
install(FILES ${DML_DIR}/LICENSE.txt DESTINATION Player/D3D12 RENAME LICENSE_DirectML.txt)
install(FILES ${DSTORAGE_DIR}/LICENSE.txt DESTINATION Player/D3D12 RENAME LICENSE_DirectStorage.txt)
