// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "intelExtensionsWrappers.h"
#include "captureManager.h"
#include "commandsCustom.h"
#include "interfaceArgumentUpdaters.h"

namespace gits {
namespace DirectX {

HRESULT INTC_D3D12_GetSupportedVersionsWrapper(
    PFNINTCDX12EXT_GETSUPPORTEDVERSIONS pfnGetSupportedVersions,
    const ID3D12Device* pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t* pSupportedExtVersionsCount) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_GetSupportedVersionsCommand command(
        GetCurrentThreadId(), pDevice, pSupportedExtVersions, pSupportedExtVersionsCount);

    updateInterface(command.pDevice_, const_cast<ID3D12Device*>(pDevice));
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnGetSupportedVersions(command.pDevice_.value, command.pSupportedExtVersions_.value,
                                       command.pSupportedExtVersionsCount_.value);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnGetSupportedVersions(pDevice, pSupportedExtVersions, pSupportedExtVersionsCount);
  }

  return result;
}

HRESULT INTC_D3D12_CreateDeviceExtensionContextWrapper(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT pfnCreateDeviceExtensionContext,
    const ID3D12Device* pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo* pExtensionInfo,
    INTCExtensionAppInfo* pExtensionAppInfo) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTCExtensionVersion version = {4, 15, 0};
    INTCExtensionInfo extensionInfo = *pExtensionInfo;
    extensionInfo.RequestedExtensionVersion = version;

    INTC_D3D12_CreateDeviceExtensionContextCommand command(
        GetCurrentThreadId(), pDevice, ppExtensionContext, &extensionInfo, pExtensionAppInfo);

    updateInterface(command.pDevice_, const_cast<ID3D12Device*>(pDevice));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCreateDeviceExtensionContext(
          command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
          command.pExtensionAppInfo_.value);
    }
    if (result == S_OK) {
      command.ppExtensionContext_.key = manager.createWrapperKey();
      manager.getIntelExtensionsContextMap().setContext(
          reinterpret_cast<std::uintptr_t>(*command.ppExtensionContext_.value),
          command.ppExtensionContext_.key);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCreateDeviceExtensionContext(pDevice, ppExtensionContext, pExtensionInfo,
                                             pExtensionAppInfo);
  }

  return result;
}

HRESULT INTC_D3D12_CreateDeviceExtensionContext1Wrapper(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT1 pfnCreateDeviceExtensionContext1,
    const ID3D12Device* pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo* pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTCExtensionVersion version = {4, 15, 0};
    INTCExtensionInfo extensionInfo = *pExtensionInfo;
    extensionInfo.RequestedExtensionVersion = version;

    INTC_D3D12_CreateDeviceExtensionContext1Command command(
        GetCurrentThreadId(), pDevice, ppExtensionContext, &extensionInfo, pExtensionAppInfo);

    updateInterface(command.pDevice_, const_cast<ID3D12Device*>(pDevice));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCreateDeviceExtensionContext1(
          command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
          command.pExtensionAppInfo_.value);
    }
    if (result == S_OK) {
      command.ppExtensionContext_.key = manager.createWrapperKey();
      manager.getIntelExtensionsContextMap().setContext(
          reinterpret_cast<std::uintptr_t>(*command.ppExtensionContext_.value),
          command.ppExtensionContext_.key);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCreateDeviceExtensionContext1(pDevice, ppExtensionContext, pExtensionInfo,
                                              pExtensionAppInfo);
  }

  return result;
}

HRESULT INTC_DestroyDeviceExtensionContextWrapper(
    PFNINTCEXT_DESTROYDEVICEEXTENSIONCONTEXT pfnDestroyDeviceExtensionContext,
    INTCExtensionContext** ppExtensionContext) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_DestroyDeviceExtensionContextCommand command(GetCurrentThreadId(), ppExtensionContext);

    auto context = reinterpret_cast<std::uintptr_t>(*command.ppExtensionContext_.value);
    command.ppExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(context);
    manager.getIntelExtensionsContextMap().removeContext(context);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnDestroyDeviceExtensionContext(command.ppExtensionContext_.value);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnDestroyDeviceExtensionContext(ppExtensionContext);
  }

  return result;
}

HRESULT INTC_D3D12_CreateCommandQueueWrapper(
    PFNINTCDX12EXT_CREATECOMMANDQUEUE pfnCreateCommandQueue,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID riid,
    void** ppCommandQueue) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateCommandQueue not handled.";
    logged = true;
  }
  HRESULT result = pfnCreateCommandQueue(pExtensionContext, pDesc, riid, ppCommandQueue);
  return result;
}

HRESULT INTC_D3D12_CreateComputePipelineStateWrapper(
    PFNINTCDX12EXT_CREATECOMPUTEPIPELINESTATE pfnCreateComputePipelineState,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_CreateComputePipelineStateCommand command(GetCurrentThreadId(), pExtensionContext,
                                                         pDesc, riid, ppPipelineState);

    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));
    UpdateInterface<PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>,
                    INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>
        update_pDesc(command.pDesc_, pDesc);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCreateComputePipelineState(command.pExtensionContext_.value, command.pDesc_.value,
                                             command.riid_.value, command.ppPipelineState_.value);
    }

    UpdateOutputInterface<InterfaceOutputArgument<void>, void> update_ppPipelineState(
        command.ppPipelineState_, result, riid, ppPipelineState);
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCreateComputePipelineState(pExtensionContext, pDesc, riid, ppPipelineState);
  }
  return result;
}

HRESULT INTC_D3D12_CreateReservedResourceWrapper(
    PFNINTCDX12EXT_CREATERESERVEDRESOURCE pfnCreateReservedResource,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateReservedResource not handled.";
    logged = true;
  }
  HRESULT result = pfnCreateReservedResource(pExtensionContext, pDesc, InitialState,
                                             pOptimizedClearValue, riid, ppvResource);
  return result;
}

HRESULT INTC_D3D12_CreateCommittedResourceWrapper(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE pfnCreateCommittedResource,
    INTCExtensionContext* pExtensionContext,
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riidResource,
    void** ppvResource) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_CreateCommittedResourceCommand command(
        GetCurrentThreadId(), pExtensionContext, pHeapProperties, HeapFlags, pDesc,
        InitialResourceState, pOptimizedClearValue, riidResource, ppvResource);

    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCreateCommittedResource(pExtensionContext, pHeapProperties, HeapFlags, pDesc,
                                          InitialResourceState, pOptimizedClearValue, riidResource,
                                          ppvResource);
    }

    UpdateOutputInterface<InterfaceOutputArgument<void>, void> update_ppvResource(
        command.ppvResource_, result, riidResource, ppvResource);
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCreateCommittedResource(pExtensionContext, pHeapProperties, HeapFlags, pDesc,
                                        InitialResourceState, pOptimizedClearValue, riidResource,
                                        ppvResource);
  }
  return result;
}

HRESULT INTC_D3D12_CreateCommittedResource1Wrapper(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE1 pfnCreateCommittedResource1,
    INTCExtensionContext* pExtensionContext,
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0002* pDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riidResource,
    void** ppvResource) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateCommittedResource1 not handled.";
    logged = true;
  }
  HRESULT result = pfnCreateCommittedResource1(pExtensionContext, pHeapProperties, HeapFlags, pDesc,
                                               InitialResourceState, pOptimizedClearValue,
                                               riidResource, ppvResource);
  return result;
}

HRESULT INTC_D3D12_CreateHeapWrapper(PFNINTCDX12EXT_CREATEHEAP pfnCreateHeap,
                                     INTCExtensionContext* pExtensionContext,
                                     const INTC_D3D12_HEAP_DESC* pDesc,
                                     REFIID riid,
                                     void** ppvHeap) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateHeap not handled.";
    logged = true;
  }
  HRESULT result = pfnCreateHeap(pExtensionContext, pDesc, riid, ppvHeap);
  return result;
}

HRESULT INTC_D3D12_CreatePlacedResourceWrapper(
    PFNINTCDX12EXT_CREATEPLACEDRESOURCE pfnCreatePlacedResource,
    INTCExtensionContext* pExtensionContext,
    ID3D12Heap* pHeap,
    UINT64 HeapOffset,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_CreatePlacedResourceCommand command(GetCurrentThreadId(), pExtensionContext, pHeap,
                                                   HeapOffset, pDesc, InitialState,
                                                   pOptimizedClearValue, riid, ppvResource);

    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));
    updateInterface(command.pHeap_, pHeap);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCreatePlacedResource(
          command.pExtensionContext_.value, command.pHeap_.value, command.HeapOffset_.value,
          command.pDesc_.value, command.InitialState_.value, command.pOptimizedClearValue_.value,
          command.riid_.value, command.ppvResource_.value);
    }

    UpdateOutputInterface<InterfaceOutputArgument<void>, void> update_ppvResource(
        command.ppvResource_, result, riid, ppvResource);
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCreatePlacedResource(pExtensionContext, pHeap, HeapOffset, pDesc, InitialState,
                                     pOptimizedClearValue, riid, ppvResource);
  }
  return result;
}

HRESULT INTC_D3D12_CreateHostRTASResourceWrapper(
    PFNINTCDX12EXT_CREATEHOSTRTASRESOURCE pfnCreateHostRTASResource,
    INTCExtensionContext* pExtensionContext,
    size_t SizeInBytes,
    DWORD Flags,
    REFIID riidResource,
    void** ppvResource) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateHostRTASResource not handled.";
    logged = true;
  }
  HRESULT result =
      pfnCreateHostRTASResource(pExtensionContext, SizeInBytes, Flags, riidResource, ppvResource);
  return result;
}

void INTC_D3D12_BuildRaytracingAccelerationStructure_HostWrapper(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST
        pfnBuildRaytracingAccelerationStructure_Host,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    const D3D12_GPU_VIRTUAL_ADDRESS* pInstanceGPUVAs,
    UINT NumInstances) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_BuildRaytracingAccelerationStructure_Host not handled.";
    logged = true;
  }
  pfnBuildRaytracingAccelerationStructure_Host(pExtensionContext, pDesc, pInstanceGPUVAs,
                                               NumInstances);
}

void INTC_D3D12_CopyRaytracingAccelerationStructure_HostWrapper(
    PFNINTCDX12EXT_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST
        pfnCopyRaytracingAccelerationStructure_Host,
    INTCExtensionContext* pExtensionContext,
    void* DestAccelerationStructureData,
    const void* SourceAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CopyRaytracingAccelerationStructure_Host not handled.";
    logged = true;
  }
  pfnCopyRaytracingAccelerationStructure_Host(pExtensionContext, DestAccelerationStructureData,
                                              SourceAccelerationStructureData, Mode);
}

void INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_HostWrapper(
    PFNINTCDX12EXT_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST
        pfnEmitRaytracingAccelerationStructurePostbuildInfo_Host,
    INTCExtensionContext* pExtensionContext,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE InfoType,
    void* DestBuffer,
    const void* SourceRTAS) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_Host not handled.";
    logged = true;
  }
  pfnEmitRaytracingAccelerationStructurePostbuildInfo_Host(pExtensionContext, InfoType, DestBuffer,
                                                           SourceRTAS);
}

void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_HostWrapper(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST
        pfnGetRaytracingAccelerationStructurePrebuildInfo_Host,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_Host not handled.";
    logged = true;
  }
  pfnGetRaytracingAccelerationStructurePrebuildInfo_Host(pExtensionContext, pDesc, pInfo);
}

void INTC_D3D12_TransferHostRTASWrapper(PFNINTCDX12EXT_TRANSFERHOSTRTAS pfnTransferHostRTAS,
                                        INTCExtensionContext* pExtensionContext,
                                        ID3D12GraphicsCommandList* pCommandList,
                                        D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData,
                                        D3D12_GPU_VIRTUAL_ADDRESS SrcAccelerationStructureData,
                                        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_TransferHostRTAS not handled.";
    logged = true;
  }
  pfnTransferHostRTAS(pExtensionContext, pCommandList, DestAccelerationStructureData,
                      SrcAccelerationStructureData, Mode);
}

void INTC_D3D12_SetDriverEventMetadataWrapper(
    PFNINTCDX12EXT_SETDRIVEREVENTMETADATA pfnSetDriverEventMetadata,
    INTCExtensionContext* pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    UINT64 Metadata) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_SetDriverEventMetadata not handled.";
    logged = true;
  }
  pfnSetDriverEventMetadata(pExtensionContext, pCommandList, Metadata);
}

void INTC_D3D12_QueryCpuVisibleVidmemWrapper(
    PFNINTCDX12EXT_QUERYCPUVISIBLEVIDMEM pfnQueryCpuVisibleVidmem,
    INTCExtensionContext* pExtensionContext,
    UINT64* pTotalBytes,
    UINT64* pFreeBytes) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_QueryCpuVisibleVidmem not handled.";
    logged = true;
  }
  pfnQueryCpuVisibleVidmem(pExtensionContext, pTotalBytes, pFreeBytes);
}

HRESULT INTC_D3D12_CreateStateObjectWrapper(PFNINTCDX12EXT_CREATESTATEOBJECT pfnCreateStateObject,
                                            INTCExtensionContext* pExtensionContext,
                                            const INTC_D3D12_STATE_OBJECT_DESC* pDesc,
                                            REFIID riid,
                                            void** ppPipelineState) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_CreateStateObject not handled.";
    logged = true;
  }
  HRESULT result = pfnCreateStateObject(pExtensionContext, pDesc, riid, ppPipelineState);
  return result;
}

void INTC_D3D12_BuildRaytracingAccelerationStructureWrapper(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE pfnBuildRaytracingAccelerationStructure,
    INTCExtensionContext* pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    UINT NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* pPostbuildInfoDescs,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA*
        pComparisonDataDesc) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_BuildRaytracingAccelerationStructure not handled.";
    logged = true;
  }
  pfnBuildRaytracingAccelerationStructure(pExtensionContext, pCommandList, pDesc,
                                          NumPostbuildInfoDescs, pPostbuildInfoDescs,
                                          pComparisonDataDesc);
}

void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfoWrapper(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO
        pfnGetRaytracingAccelerationStructurePrebuildInfo,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA*
        pComparisonDataDesc) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo not handled.";
    logged = true;
  }
  pfnGetRaytracingAccelerationStructurePrebuildInfo(pExtensionContext, pDesc, pInfo,
                                                    pComparisonDataDesc);
}

HRESULT INTC_D3D12_SetFeatureSupportWrapper(PFNINTCDX12EXT_SETFEATURESUPPORT pfnSetFeatureSupport,
                                            INTCExtensionContext* pExtensionContext,
                                            INTC_D3D12_FEATURE* pFeature) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_SetFeatureSupportCommand command(GetCurrentThreadId(), pExtensionContext, pFeature);

    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnSetFeatureSupport(command.pExtensionContext_.value, command.pFeature_.value);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnSetFeatureSupport(pExtensionContext, pFeature);
  }
  return result;
}

D3D12_RESOURCE_ALLOCATION_INFO INTC_D3D12_GetResourceAllocationInfoWrapper(
    PFNINTCDX12EXT_GETRESOURCEALLOCATIONINFO pfnGetResourceAllocationInfo,
    INTCExtensionContext* pExtensionContext,
    UINT visibleMask,
    UINT numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs) {

  D3D12_RESOURCE_ALLOCATION_INFO result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_GetResourceAllocationInfoCommand command(
        GetCurrentThreadId(), pExtensionContext, visibleMask, numResourceDescs, pResourceDescs);
    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnGetResourceAllocationInfo(
          command.pExtensionContext_.value, command.visibleMask_.value,
          command.numResourceDescs_.value, command.pResourceDescs_.value);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnGetResourceAllocationInfo(pExtensionContext, visibleMask, numResourceDescs,
                                          pResourceDescs);
  }
  return result;
}

HRESULT INTC_D3D12_CheckFeatureSupportWrapper(
    PFNINTCDX12EXT_CHECKFEATURESUPPORT pfnCheckFeatureSupport,
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURES Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize) {

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    INTC_D3D12_CheckFeatureSupportCommand command(GetCurrentThreadId(), pExtensionContext, Feature,
                                                  pFeatureSupportData, FeatureSupportDataSize);
    command.pExtensionContext_.key = manager.getIntelExtensionsContextMap().getKey(
        reinterpret_cast<std::uintptr_t>(command.pExtensionContext_.value));

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = pfnCheckFeatureSupport(command.pExtensionContext_.value, command.Feature_.value,
                                      command.pFeatureSupportData_.value,
                                      command.FeatureSupportDataSize_.value);
    }
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = pfnCheckFeatureSupport(pExtensionContext, Feature, pFeatureSupportData,
                                    FeatureSupportDataSize);
  }

  return result;
}

HRESULT INTC_D3D12_AddShaderBinariesPathWrapper(
    PFNINTCDX12EXT_ADDSHADERBINARIESPATH pfnAddShaderBinariesPath,
    INTCExtensionContext* pExtensionContext,
    const wchar_t* filePath) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_AddShaderBinariesPath not handled.";
    logged = true;
  }
  HRESULT result = pfnAddShaderBinariesPath(pExtensionContext, filePath);
  return result;
}

HRESULT INTC_D3D12_RemoveShaderBinariesPathWrapper(
    PFNINTCDX12EXT_REMOVESHADERBINARIESPATH pfnRemoveShaderBinariesPath,
    INTCExtensionContext* pExtensionContext,
    const wchar_t* filePath) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_RemoveShaderBinariesPath not handled.";
    logged = true;
  }
  HRESULT result = pfnRemoveShaderBinariesPath(pExtensionContext, filePath);
  return result;
}

HRESULT INTC_D3D12_SetApplicationInfoWrapper(
    PFNINTCDX12EXT_SETAPPLICATIONINFO pfnSetApplicationInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "INTC_D3D12_SetApplicationInfo not handled.";
    logged = true;
  }
  HRESULT result = pfnSetApplicationInfo(pExtensionAppInfo);
  return result;
}

} // namespace DirectX
} // namespace gits
