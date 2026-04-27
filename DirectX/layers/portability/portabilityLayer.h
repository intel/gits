// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "resourcePlacementCapture.h"
#include "resourcePlacementCaptureNoExecute.h"
#include "resourcePlacementPlayback.h"
#include "resourcePlacementAssertions.h"

#include <functional>
#include <set>
#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

class PortabilityLayer : public Layer {

public:
  using ResourceRegistrationCallback =
      std::function<void(unsigned resourceKey, ID3D12Resource* resource)>;

  PortabilityLayer();
  PortabilityLayer(ResourceRegistrationCallback registerResource);
  ~PortabilityLayer();

public:
  void Pre(D3D12CreateDeviceCommand& c) override;
  void Pre(ID3D12DeviceCreateHeapCommand& c) override;
  void Post(ID3D12DeviceCreateHeapCommand& c) override;
  void Pre(ID3D12Device4CreateHeap1Command& c) override;
  void Post(ID3D12Device4CreateHeap1Command& c) override;
  void Pre(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Pre(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Pre(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) override;
  void Post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c)
      override;
  void Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;
  void Pre(ID3D12DeviceGetResourceAllocationInfoCommand& c) override;
  void Pre(ID3D12Device4GetResourceAllocationInfo1Command& c) override;
  void Pre(ID3D12Device8GetResourceAllocationInfo2Command& c) override;
  void Pre(ID3D12Device12GetResourceAllocationInfo3Command& c) override;
  void Pre(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;

private:
  void ConfigureHeapMemoryPool(ID3D12Device* device, D3D12_HEAP_DESC* heapDesc);
  void CheckHeapCreationFlags(unsigned heapKey, ID3D12Device* device, D3D12_HEAP_DESC* desc);

  HRESULT CreateCommittedResource(ID3D12Heap* heap,
                                  const D3D12_RESOURCE_DESC* desc,
                                  D3D12_RESOURCE_STATES initialState,
                                  const D3D12_CLEAR_VALUE* clearValue,
                                  REFIID riid,
                                  void** ppvResource);
  HRESULT CreateCommittedResource2(ID3D12Heap* heap,
                                   const D3D12_RESOURCE_DESC1* desc,
                                   D3D12_RESOURCE_STATES initialState,
                                   const D3D12_CLEAR_VALUE* clearValue,
                                   REFIID riid,
                                   void** ppvResource);
  HRESULT CreateCommittedResource3(ID3D12Heap* heap,
                                   const D3D12_RESOURCE_DESC1* desc,
                                   D3D12_BARRIER_LAYOUT initialLayout,
                                   const D3D12_CLEAR_VALUE* clearValue,
                                   UINT32 numCastableFormats,
                                   const DXGI_FORMAT* pCastableFormats,
                                   REFIID riid,
                                   void** ppvResource);

  void TransitionResource(ID3D12Device* device,
                          ID3D12Resource* resource,
                          D3D12_RESOURCE_STATES stateBefore,
                          D3D12_RESOURCE_STATES stateAfter);
  D3D12_RESOURCE_STATES BarrierLayoutToResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_HEAP_FLAGS GetCompatibleHeapFlags(const D3D12_HEAP_DESC& heapDesc,
                                          const D3D12_RESOURCE_DESC& desc);

  void AdjustResourceFlagsForCommitted(const D3D12_HEAP_DESC& heapDesc,
                                       const D3D12_RESOURCE_DESC& desc,
                                       D3D12_RESOURCE_FLAGS& resourceFlags);

  D3D12_HEAP_PROPERTIES GetCompatibleHeapProperties(const D3D12_HEAP_DESC& heapDesc,
                                                    const D3D12_RESOURCE_DESC& adjustedDesc,
                                                    D3D12_RESOURCE_STATES creationState);

  D3D12_RESOURCE_STATES GetCompatibleInitialState(const D3D12_HEAP_PROPERTIES& heapProps,
                                                  const D3D12_RESOURCE_DESC& adjustedDesc,
                                                  D3D12_RESOURCE_STATES originalInitialState);

  const D3D12_CLEAR_VALUE* GetCompatibleClearValue(const D3D12_RESOURCE_DESC& adjustedDesc,
                                                   const D3D12_CLEAR_VALUE* originalClearValue);
  bool CanReplacePlacedResourceWithCommitted(const D3D12_HEAP_DESC& heapDesc,
                                             const D3D12_RESOURCE_DESC& resourceDesc) const;
  std::string GetPlacedToCommittedIncompatibilityReasons(
      const D3D12_HEAP_DESC& heapDesc, const D3D12_RESOURCE_DESC& resourceDesc) const;
  std::string GetPlacedToCommittedFailureContext(const char* apiName,
                                                 unsigned commandKey,
                                                 unsigned heapKey,
                                                 const D3D12_HEAP_DESC& heapDesc,
                                                 const D3D12_RESOURCE_DESC& resourceDesc) const;
  void FailPlacedToCommittedIncompatibility(const char* apiName,
                                            unsigned commandKey,
                                            unsigned heapKey,
                                            const D3D12_HEAP_DESC& heapDesc,
                                            const D3D12_RESOURCE_DESC& resourceDesc) const;
  void FailPlacedToCommittedCreation(const char* apiName,
                                     unsigned commandKey,
                                     unsigned heapKey,
                                     UINT64 heapOffset,
                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                     D3D12_RESOURCE_STATES initialState,
                                     HRESULT hr) const;
  void FailPlacedToCommittedCreation(const char* apiName,
                                     unsigned commandKey,
                                     unsigned heapKey,
                                     UINT64 heapOffset,
                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                     D3D12_BARRIER_LAYOUT initialLayout,
                                     HRESULT hr) const;

  template <typename CommandType>
  void RegisterForcedPlacedResource(CommandType& c);

private:
  ResourceRegistrationCallback m_RegisterResource;
  ResourcePlacementCapture m_ResourcePlacementCapture;
  ResourcePlacementCaptureNoExecute m_ResourcePlacementCaptureNoExecute;
  ResourcePlacementPlayback m_ResourcePlacementPlayback;
  ResourcePlacementAssertions m_ResourcePlacementAssertions;
  bool m_StoreResourcePlacementData{};
  bool m_StoreResourcePlacementDataNoExecute{};
  bool m_UseResourcePlacementData{};
  bool m_PortabilityChecks{};
  bool m_PortabilityAssertions{};
  bool m_ForcePlacedToCommittedResources{};
  float m_AccelerationStructurePadding{1.0};
  float m_AccelerationStructureScratchPadding{1.0};
  std::unordered_set<unsigned> m_ForcedCommittedResources{};
};

} // namespace DirectX
} // namespace gits
