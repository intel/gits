// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "resourceUsageTrackingService.h"
#include "analyzerResults.h"
#include "resourceForCBVRestoreService.h"
#include "keyUtils.h"

#include <vector>
#include <unordered_map>
#include <queue>

namespace gits {
namespace DirectX {
class INTC_D3D12_SetApplicationInfoCommand;

class StateTrackingService {
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
      AnalyzerResults& analyzerResults,
      ResourceUsageTrackingService& resourceUsageTrackingService,
      ResourceForCBVRestoreService& resourceForCBVRestoreService,
      XellStateService& xellStateService,
      XefgStateService& xefgStateService)
      : m_Recorder(recorder),
        m_ResourceContentRestore(*this),
        m_SwapChainService(*this),
        m_AnalyzerResults(analyzerResults),
        m_FenceTrackingService(fenceTrackingService),
        m_MapStateService(mapStateService),
        m_ResourceStateTrackingService(resourceStateTrackingService),
        m_ReservedResourcesService(reservedResourcesService),
        m_DescriptorService(descriptorService),
        m_CommandListService(commandListService),
        m_CommandQueueService(commandQueueService),
        m_XessStateService(xessStateService),
        m_AccelerationStructuresSerializeService(accelerationStructuresSerializeService),
        m_AccelerationStructuresBuildService(accelerationStructuresBuildService),
        m_ResidencyService(residencyService),
        m_ResourceUsageTrackingService(resourceUsageTrackingService),
        m_ResourceForCBVRestoreService(resourceForCBVRestoreService),
        m_NvapiGlobalStateService(*this),
        m_XellStateService(xellStateService),
        m_XefgStateService(xefgStateService) {}
  ~StateTrackingService();
  StateTrackingService(StateTrackingService&) = delete;
  StateTrackingService& operator=(StateTrackingService&) = delete;

  void RestoreState();
  void KeepState(unsigned objectKey);
  void StoreState(ObjectState* state);
  void RemoveState(unsigned key);
  void StoreINTCFeature(INTC_D3D12_FEATURE feature);
  void StoreINTCApplicationInfo(INTC_D3D12_SetApplicationInfoCommand& c);
  void StoreD3D12EnableExperimentalFeatures(const D3D12EnableExperimentalFeaturesCommand& c);
  void StoreDllContainer(const DllContainerMetaCommand& c);
  void ReleaseObject(unsigned key, ULONG result);
  void SetReferenceCount(unsigned objectKey, ULONG referenceCount);
  ObjectState* GetState(unsigned key);
  void RestoreState(unsigned key);
  bool StateRestored(unsigned key);
  void AddBackBuffer(unsigned buffer, unsigned ResourceKey, ID3D12Resource* resource);
  void SetXefgSwapChainFlag();

  unsigned GetUniqueCommandKey() {
    return ++m_RestoreCommandKey;
  };
  unsigned GetUniqueObjectKey() {
    return ++m_RestoreObjectKey;
  };
  void* GetUniqueFakePointer() {
    return reinterpret_cast<void*>(++m_RestoreFakePointer);
  };
  SubcaptureRecorder& GetRecorder() {
    return m_Recorder;
  }
  AnalyzerResults& GetAnalyzerResults() {
    return m_AnalyzerResults;
  }
  DescriptorService& GetDescriptorService() {
    return m_DescriptorService;
  }
  ResourceStateTrackingService& GetResourceStateTrackingService() {
    return m_ResourceStateTrackingService;
  }
  unsigned GetDeviceKey() {
    return m_DeviceKey;
  }

  class NvAPIGlobalStateService {
  public:
    NvAPIGlobalStateService(StateTrackingService& stateService) : m_StateService(stateService) {}
    void IncrementInitialize();
    void DecrementInitialize();
    void AddSetCreatePipelineStateOptionsCommand(
        const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& Command);
    void AddSetNvShaderExtnSlotSpaceCommand(
        const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& Command);
    void AddSetNvShaderExtnSlotSpaceLocalThreadCommand(
        const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& Command);
    void RestoreInitializeCount();
    void RestoreCreatePipelineStateOptionsBeforeCommand(unsigned commandKey);
    void RestoreShaderExtnSlotSpaceBeforeCommand(unsigned commandKey);
    void FinalizeRestore();

  private:
    struct OrderedCommand {
      unsigned Key{};
      std::unique_ptr<Command> SerializedCommand;
    };

    struct CompareCommand {
      bool operator()(const OrderedCommand& a, const OrderedCommand& b) const {
        return a.Key > b.Key; // reverse order for min-heap
      }
    };

    StateTrackingService& m_StateService;
    unsigned m_NvapiInitializeCount{};
    std::priority_queue<OrderedCommand, std::vector<OrderedCommand>, CompareCommand>
        m_SetCreatePipelineStateOptionsCommands;
    std::priority_queue<OrderedCommand, std::vector<OrderedCommand>, CompareCommand>
        m_SetNvShaderExtnSlotSpaceCommands;
  };

  NvAPIGlobalStateService& GetNvAPIGlobalStateService() {
    return m_NvapiGlobalStateService;
  }

private:
  void RestoreState(ObjectState* state);
  void RestoreReferenceCount();
  void RestoreResources();
  D3D12_RESOURCE_STATES GetResourceInitialState(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension);
  D3D12_BARRIER_LAYOUT GetResourceInitialLayout(ResourceState& state,
                                                D3D12_RESOURCE_DIMENSION dimension,
                                                D3D12_RESOURCE_FLAGS flags);
  void RestoreINTCApplicationInfo();
  void RestoreD3D12EnableExperimentalFeatures();
  void RestoreDllContainers();
  void RestoreStateObjectProperties();
  void RestoreResidencyPriority(unsigned DeviceKey,
                                unsigned objectKey,
                                D3D12_RESIDENCY_PRIORITY residencyPriority);
  void RestoreDXGISwapChain(ObjectState* state);
  void RestoreDXGIAdapter(ObjectState* state);
  void RestoreD3D12DescriptorHeap(ObjectState* state);
  void RestoreD3D12Device(ObjectState* state);
  void RestoreQueryInterface(ObjectState* state);
  void RestoreD3D12Fence(ObjectState* state);
  void RestoreD3D12CommandList(ObjectState* state);
  void RestoreD3D12Heap(ObjectState* state);
  void RestoreD3D12HeapFromAddress(ObjectState* state);
  void RestoreD3D12CommittedResource(ObjectState* state);
  void RestoreD3D12PlacedResource(ObjectState* state);
  void RestoreD3D12ReservedResource(ObjectState* state);
  void RestoreGpuVirtualAddress(ResourceState* state);
  void RestoreD3D12INTCDeviceExtensionContext(ObjectState* state);
  void RestoreD3D12StateObject(ObjectState* state);
  void RestoreD3D12PipelineStateObject(ObjectState* state);

private:
  SubcaptureRecorder& m_Recorder;
  ResourceContentRestore m_ResourceContentRestore;
  std::map<unsigned, ObjectState*> m_StatesByKey;
  unsigned m_RestoreCommandKey{STATE_RESTORE_KEY_MASK};
  unsigned m_RestoreObjectKey{STATE_RESTORE_KEY_MASK};
  unsigned m_RestoreFakePointer{};
  AnalyzerResults& m_AnalyzerResults;
  FenceTrackingService& m_FenceTrackingService;
  MapStateService& m_MapStateService;
  ResourceStateTrackingService& m_ResourceStateTrackingService;
  ReservedResourcesService& m_ReservedResourcesService;
  DescriptorService& m_DescriptorService;
  CommandListService& m_CommandListService;
  CommandQueueService& m_CommandQueueService;
  XessStateService& m_XessStateService;
  XellStateService& m_XellStateService;
  XefgStateService& m_XefgStateService;
  AccelerationStructuresSerializeService& m_AccelerationStructuresSerializeService;
  AccelerationStructuresBuildService& m_AccelerationStructuresBuildService;
  ResidencyService& m_ResidencyService;
  ResourceUsageTrackingService& m_ResourceUsageTrackingService;
  ResourceForCBVRestoreService& m_ResourceForCBVRestoreService;
  unsigned m_DeviceKey{};
  INTC_D3D12_FEATURE m_IntcFeature{};
  std::unique_ptr<INTC_D3D12_SetApplicationInfoCommand> m_SetApplicationInfoCommand;
  std::unique_ptr<D3D12EnableExperimentalFeaturesCommand> m_EnableExperimentalFeaturesCommand;
  std::vector<std::unique_ptr<DllContainerMetaCommand>> m_DllContainerCommands;
  std::queue<std::unique_ptr<Command>> m_StateObjectPropertiesCommands;
  NvAPIGlobalStateService m_NvapiGlobalStateService;
  bool m_IsXefgSwapChain{};

private:
  class SwapChainService {
  public:
    SwapChainService(StateTrackingService& stateService) : m_StateService(stateService) {}
    void SetSwapChain(unsigned commandQueueKey,
                      ID3D12CommandQueue* commandQueue,
                      unsigned swapChainKey,
                      IDXGISwapChain* swapChain,
                      unsigned backBuffersCount);
    void RestoreBackBufferSequence(bool CommandListSubcapture);
    void RecordSwapChainPresent();
    void AddBackBuffer(unsigned buffer, unsigned ResourceKey, ID3D12Resource* resource);
    unsigned GetBackBuffersCount() {
      return m_BackBuffersCount;
    }

  private:
    StateTrackingService& m_StateService;
    unsigned m_SwapChainKey{};
    ID3D12CommandQueue* m_CommandQueue{};
    unsigned m_CommandQueueKey{};
    IDXGISwapChain* m_SwapChain{};
    unsigned m_BackBufferShift{};
    unsigned m_BackBuffersCount{};
    std::unordered_map<unsigned, std::pair<unsigned, ID3D12Resource*>> m_BackBuffers;
  };
  SwapChainService m_SwapChainService;
};

} // namespace DirectX
} // namespace gits
