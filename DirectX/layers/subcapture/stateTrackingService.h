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
#include "commandQueueService.h"
#include "xessStateService.h"
#include "accelerationStructuresBuildService.h"
#include "accelerationStructuresSerializeService.h"
#include "residencyService.h"
#include "analyzerResults.h"

#include <vector>
#include <unordered_map>

namespace gits {
namespace DirectX {
class INTC_D3D12_SetApplicationInfoCommand;

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
      CommandQueueService& commandQueueService,
      XessStateService& xessStateService,
      AccelerationStructuresSerializeService& accelerationStructuresSerializeService,
      AccelerationStructuresBuildService& accelerationStructuresBuildService,
      ResidencyService& residencyService,
      AnalyzerResults& analyzerResults)
      : recorder_(recorder),
        resourceContentRestore_(*this),
        swapChainService_(*this),
        analyzerResults_(analyzerResults),
        fenceTrackingService_(fenceTrackingService),
        mapStateService_(mapStateService),
        resourceStateTrackingService_(resourceStateTrackingService),
        reservedResourcesService_(reservedResourcesService),
        descriptorService_(descriptorService),
        commandListService_(commandListService),
        commandQueueService_(commandQueueService),
        xessStateService_(xessStateService),
        accelerationStructuresSerializeService_(accelerationStructuresSerializeService),
        accelerationStructuresBuildService_(accelerationStructuresBuildService),
        residencyService_(residencyService) {}
  ~StateTrackingService();
  void restoreState();
  void keepState(unsigned objectKey);
  void storeState(ObjectState* state);
  void storeINTCFeature(INTC_D3D12_FEATURE feature);
  void storeINTCApplicationInfo(INTC_D3D12_SetApplicationInfoCommand& c);
  void releaseObject(unsigned key, ULONG result);
  void setReferenceCount(unsigned objectKey, ULONG referenceCount);
  ObjectState* getState(unsigned key);

  unsigned getUniqueCommandKey() {
    return ++restoreCommandKey_;
  };
  unsigned getUniqueObjectKey() {
    return ++restoreObjectKey_;
  };
  void* getUniqueFakePointer() {
    return reinterpret_cast<void*>(++restoreFakePointer_);
  };
  SubcaptureRecorder& getRecorder() {
    return recorder_;
  }
  AnalyzerResults& getAnalyzerResults() {
    return analyzerResults_;
  }
  DescriptorService& getDescriptorService() {
    return descriptorService_;
  }
  ResourceStateTrackingService& getResourceStateTrackingService() {
    return resourceStateTrackingService_;
  }
  unsigned getDeviceKey() {
    return deviceKey_;
  }

private:
  void copyAuxiliaryFiles();
  void restoreState(ObjectState* state);
  void restoreReferenceCount();
  D3D12_RESOURCE_STATES getResourceInitialState(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension);
  D3D12_BARRIER_LAYOUT getResourceInitialLayout(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension);
  void restoreINTCApplicationInfo();
  void restoreResidencyPriority(unsigned deviceKey,
                                unsigned objectKey,
                                D3D12_RESIDENCY_PRIORITY residencyPriority);
  void restoreDXGISwapChain(ObjectState* state);
  void restoreDXGIAdapter(ObjectState* state);
  void restoreD3D12DescriptorHeap(ObjectState* state);
  void restoreD3D12Device(ObjectState* state);
  void restoreQueryInterface(ObjectState* state);
  void restoreD3D12Fence(ObjectState* state);
  void restoreD3D12CommandList(ObjectState* state);
  void restoreD3D12Heap(ObjectState* state);
  void restoreD3D12HeapFromAddress(ObjectState* state);
  void restoreD3D12CommittedResource(ObjectState* state);
  void restoreD3D12PlacedResource(ObjectState* state);
  void restoreD3D12ReservedResource(ObjectState* state);
  void restoreGpuVirtualAddress(ResourceState* state);
  void restoreD3D12INTCDeviceExtensionContext(ObjectState* state);

private:
  SubcaptureRecorder& recorder_;
  ResourceContentRestore resourceContentRestore_;
  std::map<unsigned, ObjectState*> statesByKey_;
  unsigned restoreCommandKey_{Command::stateRestoreKeyMask};
  unsigned restoreObjectKey_{Command::stateRestoreKeyMask};
  unsigned restoreFakePointer_{};
  AnalyzerResults& analyzerResults_;
  FenceTrackingService& fenceTrackingService_;
  MapStateService& mapStateService_;
  ResourceStateTrackingService& resourceStateTrackingService_;
  ReservedResourcesService& reservedResourcesService_;
  DescriptorService& descriptorService_;
  CommandListService& commandListService_;
  CommandQueueService& commandQueueService_;
  XessStateService& xessStateService_;
  AccelerationStructuresSerializeService& accelerationStructuresSerializeService_;
  AccelerationStructuresBuildService& accelerationStructuresBuildService_;
  ResidencyService& residencyService_;
  unsigned deviceKey_{};
  INTC_D3D12_FEATURE intcFeature_{};
  std::unique_ptr<INTC_D3D12_SetApplicationInfoCommand> setApplicationInfoCommand_;

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
