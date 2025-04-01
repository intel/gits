// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRecorder.h"
#include "objectState.h"
#include "resourceContentRestore.h"
#include "fenceTrackingService.h"
#include "mapStateService.h"
#include "resourceStateTrackingService.h"
#include "reservedResourcesService.h"
#include "descriptorService.h"
#include "commandListService.h"
#include "xessStateService.h"
#include "accelerationStructuresBuildService.h"
#include "accelerationStructuresSerializeService.h"
#include "residencyService.h"

#include <vector>
#include <unordered_map>

namespace gits {
namespace DirectX {

class StateTrackingService : public gits::noncopyable {
public:
  StateTrackingService(
      SubcaptureRecorder& recorder,
      FenceTrackingService& fenceTrackingService,
      MapStateService& mapStateService,
      ResourceStateTrackingService& resourceStateTrackingService,
      ReservedResourcesService& reservedResourcesService,
      DescriptorService& descriptorService,
      CommandListService& commandListService,
      XessStateService& xessStateService,
      AccelerationStructuresSerializeService& accelerationStructuresSerializeService,
      AccelerationStructuresBuildService& accelerationStructuresBuildService,
      ResidencyService& residencyService)
      : recorder_(recorder),
        resourceContentRestore_(*this),
        swapChainService_(*this),
        fenceTrackingService_(fenceTrackingService),
        mapStateService_(mapStateService),
        resourceStateTrackingService_(resourceStateTrackingService),
        reservedResourcesService_(reservedResourcesService),
        descriptorService_(descriptorService),
        commandListService_(commandListService),
        xessStateService_(xessStateService),
        accelerationStructuresSerializeService_(accelerationStructuresSerializeService),
        accelerationStructuresBuildService_(accelerationStructuresBuildService),
        residencyService_(residencyService) {}
  ~StateTrackingService();
  void restoreState();
  void keepState(unsigned objectKey);
  void storeState(ObjectState* state);
  void storeINTCFeature(INTC_D3D12_FEATURE feature);
  void releaseObject(unsigned key, ULONG result);
  void setReferenceCount(unsigned objectKey, ULONG referenceCount);
  ObjectState* getState(unsigned key);

private:
  unsigned getUniqueCommandKey() {
    return ++restoreCommandKey_;
  };
  unsigned getUniqueObjectKey() {
    return ++restoreObjectKey_;
  };
  void* getUniqueFakePointer() {
    return reinterpret_cast<void*>(++restoreFakePointer_);
  };
  void copyAuxiliaryFiles();
  void restoreState(ObjectState* state);
  void restoreReferenceCount();
  D3D12_RESOURCE_STATES getResourceInitialState(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension);
  D3D12_BARRIER_LAYOUT getResourceInitialLayout(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension);
  void restoreResidencyPriority(unsigned deviceKey,
                                unsigned objectKey,
                                D3D12_RESIDENCY_PRIORITY residencyPriority);
  void restoreDXGIFactory(DXGIFactoryState* state);
  void restoreDXGIAdapter(DXGIAdapterState* state);
  void restoreDXGIAdapterByLuid(DXGIAdapterByLuidState* state);
  void restoreDXGIGetParent(DXGIGetParentState* state);
  void restoreSwapChain(DXGISwapChainState* state);
  void restoreSwapChainForHwnd(DXGISwapChainForHwndState* state);
  void restoreSwapChainBuffer(DXGISwapChainBufferState* state);
  void restoreD3D12Device(D3D12DeviceState* state);
  void restoreD3D12CommandQueue(D3D12CommandQueueState* state);
  void restoreD3D12CommandQueue1(D3D12CommandQueue1State* state);
  void restoreD3D12DescriptorHeap(D3D12DescriptorHeapState* state);
  void restoreD3D12Heap(D3D12HeapState* state);
  void restoreD3D12Heap1(D3D12Heap1State* state);
  void restoreD3D12HeapFromAddress(D3D12HeapFromAddressState* state);
  void restoreD3D12QueryHeap(D3D12QueryHeapState* state);
  void restoreD3D12CommandAllocator(D3D12CommandAllocatorState* state);
  void restoreD3D12RootSignature(D3D12RootSignatureState* state);
  void restoreD3D12PipelineLibrary(D3D12PipelineLibraryState* state);
  void restoreD3D12LoadPipelineState(D3D12LoadPipelineState* state);
  void restoreD3D12LoadGraphicsPipelineState(D3D12LoadGraphicsPipelineState* state);
  void restoreD3D12LoadComputePipelineState(D3D12LoadComputePipelineState* state);
  void restoreD3D12CommandSignature(D3D12CommandSignatureState* state);
  void restoreD3D12GraphicsPipelineState(D3D12GraphicsPipelineState* state);
  void restoreD3D12ComputePipelineState(D3D12ComputePipelineState* state);
  void restoreD3D12PipelineStateStreamState(D3D12PipelineStateStreamState* state);
  void restoreD3D12StateObjectState(D3D12StateObjectState* state);
  void restoreD3D12AddToStateObjectState(D3D12AddToStateObjectState* state);
  void restoreD3D12StateObjectPropertiesState(D3D12StateObjectPropertiesState* state);
  void restoreD3D12CommandList(D3D12CommandListState* state);
  void restoreD3D12CommandList1(D3D12CommandList1State* state);
  void restoreD3D12CommittedResource(D3D12CommittedResourceState* state);
  void restoreD3D12CommittedResource1(D3D12CommittedResource1State* state);
  void restoreD3D12CommittedResource2(D3D12CommittedResource2State* state);
  void restoreD3D12CommittedResource3(D3D12CommittedResource3State* state);
  void restoreD3D12PlacedResource(D3D12PlacedResourceState* state);
  void restoreD3D12PlacedResource1(D3D12PlacedResource1State* state);
  void restoreD3D12ReservedResource(D3D12ReservedResourceState* state);
  void restoreD3D12ReservedResource1(D3D12ReservedResource1State* state);
  void restoreGpuVirtualAddress(ResourceState* state);
  void restoreD3D12Fence(D3D12FenceState* state);
  void restoreD3D12INTCDeviceExtensionContext(D3D12INTCDeviceExtensionContextState* state);
  void restoreD3D12INTCDeviceExtensionContext1(D3D12INTCDeviceExtensionContext1State* state);
  void restoreD3D12INTCCommittedResource(D3D12INTCCommittedResourceState* state);
  void restoreD3D12INTCPlacedResource(D3D12INTCPlacedResourceState* state);
  void restoreD3D12INTCComputePipelineState(D3D12INTCComputePipelineState* state);
  void restoreD3D12INTCHeapState(D3D12INTCHeapState* state);
  void restoreDStorageFactoryState(DStorageFactoryState* state);
  void restoreDStorageFileState(DStorageFileState* state);
  void restoreDStorageQueueState(DStorageQueueState* state);
  void restoreDStorageCustomDecompressionQueueState(DStorageCustomDecompressionQueueState* state);
  void restoreDStorageStatusArrayState(DStorageStatusArrayState* state);

private:
  friend class ResourceContentRestore;
  friend class MapStateService;
  friend class ResourceStateTrackingService;
  friend class ReservedResourcesService;
  friend class DescriptorService;
  friend class CommandListService;
  friend class XessStateService;
  friend class AccelerationStructuresBuildService;
  friend class AccelerationStructuresSerializeService;
  friend class AccelerationStructuresBufferContentRestore;
  friend class ResidencyService;
  SubcaptureRecorder& recorder_;
  ResourceContentRestore resourceContentRestore_;
  std::map<unsigned, ObjectState*> statesByKey_;
  unsigned restoreCommandKey_{Command::stateRestoreKeyMask};
  unsigned restoreObjectKey_{Command::stateRestoreKeyMask};
  unsigned restoreFakePointer_{};
  FenceTrackingService& fenceTrackingService_;
  MapStateService& mapStateService_;
  ResourceStateTrackingService& resourceStateTrackingService_;
  ReservedResourcesService& reservedResourcesService_;
  DescriptorService& descriptorService_;
  CommandListService& commandListService_;
  XessStateService& xessStateService_;
  AccelerationStructuresSerializeService& accelerationStructuresSerializeService_;
  AccelerationStructuresBuildService& accelerationStructuresBuildService_;
  ResidencyService& residencyService_;
  unsigned deviceKey_{};
  INTC_D3D12_FEATURE intcFeature_{};

private:
  class SwapChainService {
  public:
    SwapChainService(StateTrackingService& stateService) : stateService_(stateService) {}
    void setSwapChain(unsigned key, IDXGISwapChain* swapChain, unsigned backBuffersCount);
    void restoreBackBufferSequence();
    void recordSwapChainPresent();

  private:
    StateTrackingService& stateService_;
    unsigned swapChainKey_{};
    IDXGISwapChain* swapChain_{};
    unsigned backBufferShift_{};
    unsigned backBuffersCount_{};
  };
  SwapChainService swapChainService_;
};

} // namespace DirectX
} // namespace gits
