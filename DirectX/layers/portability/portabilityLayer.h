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
  void pre(D3D12CreateDeviceCommand& c) override;
  void pre(ID3D12DeviceCreateHeapCommand& c) override;
  void post(ID3D12DeviceCreateHeapCommand& c) override;
  void pre(ID3D12Device4CreateHeap1Command& c) override;
  void post(ID3D12Device4CreateHeap1Command& c) override;
  void pre(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void pre(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void pre(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) override;
  void post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c)
      override;
  void post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;
  void pre(ID3D12DeviceGetResourceAllocationInfoCommand& c) override;
  void pre(ID3D12Device4GetResourceAllocationInfo1Command& c) override;
  void pre(ID3D12Device8GetResourceAllocationInfo2Command& c) override;
  void pre(ID3D12Device12GetResourceAllocationInfo3Command& c) override;
  void pre(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;

private:
  void configureHeapMemoryPool(ID3D12Device* device, D3D12_HEAP_DESC* heapDesc);
  void checkHeapCreationFlags(unsigned heapKey, ID3D12Device* device, D3D12_HEAP_DESC* desc);

  HRESULT createCommittedResource(ID3D12Heap* heap,
                                  const D3D12_RESOURCE_DESC* desc,
                                  D3D12_RESOURCE_STATES initialState,
                                  const D3D12_CLEAR_VALUE* clearValue,
                                  REFIID riid,
                                  void** ppvResource);
  HRESULT createCommittedResource2(ID3D12Heap* heap,
                                   const D3D12_RESOURCE_DESC1* desc,
                                   D3D12_RESOURCE_STATES initialState,
                                   const D3D12_CLEAR_VALUE* clearValue,
                                   REFIID riid,
                                   void** ppvResource);
  HRESULT createCommittedResource3(ID3D12Heap* heap,
                                   const D3D12_RESOURCE_DESC1* desc,
                                   D3D12_BARRIER_LAYOUT initialLayout,
                                   const D3D12_CLEAR_VALUE* clearValue,
                                   UINT32 numCastableFormats,
                                   const DXGI_FORMAT* pCastableFormats,
                                   REFIID riid,
                                   void** ppvResource);

  void transitionResource(ID3D12Device* device,
                          ID3D12Resource* resource,
                          D3D12_RESOURCE_STATES stateBefore,
                          D3D12_RESOURCE_STATES stateAfter);
  D3D12_RESOURCE_STATES barrierLayoutToResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_HEAP_FLAGS getCompatibleHeapFlags(const D3D12_HEAP_DESC& heapDesc,
                                          const D3D12_RESOURCE_DESC& desc);

  void adjustResourceFlagsForCommitted(const D3D12_HEAP_DESC& heapDesc,
                                       const D3D12_RESOURCE_DESC& desc,
                                       D3D12_RESOURCE_FLAGS& resourceFlags);

  D3D12_HEAP_PROPERTIES getCompatibleHeapProperties(const D3D12_HEAP_DESC& heapDesc,
                                                    const D3D12_RESOURCE_DESC& adjustedDesc,
                                                    D3D12_RESOURCE_STATES creationState);

  D3D12_RESOURCE_STATES getCompatibleInitialState(const D3D12_HEAP_PROPERTIES& heapProps,
                                                  const D3D12_RESOURCE_DESC& adjustedDesc,
                                                  D3D12_RESOURCE_STATES originalInitialState);

  const D3D12_CLEAR_VALUE* getCompatibleClearValue(const D3D12_RESOURCE_DESC& adjustedDesc,
                                                   const D3D12_CLEAR_VALUE* originalClearValue);
  bool canReplacePlacedResourceWithCommitted(const D3D12_HEAP_DESC& heapDesc,
                                             const D3D12_RESOURCE_DESC& resourceDesc) const;
  std::string getPlacedToCommittedIncompatibilityReasons(
      const D3D12_HEAP_DESC& heapDesc, const D3D12_RESOURCE_DESC& resourceDesc) const;
  std::string getPlacedToCommittedFailureContext(const char* apiName,
                                                 unsigned commandKey,
                                                 unsigned heapKey,
                                                 const D3D12_HEAP_DESC& heapDesc,
                                                 const D3D12_RESOURCE_DESC& resourceDesc) const;
  void failPlacedToCommittedIncompatibility(const char* apiName,
                                            unsigned commandKey,
                                            unsigned heapKey,
                                            const D3D12_HEAP_DESC& heapDesc,
                                            const D3D12_RESOURCE_DESC& resourceDesc) const;
  void failPlacedToCommittedCreation(const char* apiName,
                                     unsigned commandKey,
                                     unsigned heapKey,
                                     UINT64 heapOffset,
                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                     D3D12_RESOURCE_STATES initialState,
                                     HRESULT hr) const;
  void failPlacedToCommittedCreation(const char* apiName,
                                     unsigned commandKey,
                                     unsigned heapKey,
                                     UINT64 heapOffset,
                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                     D3D12_BARRIER_LAYOUT initialLayout,
                                     HRESULT hr) const;

  template <typename CommandType>
  void registerForcedPlacedResource(CommandType& c);

private:
  ResourceRegistrationCallback registerResource_;
  ResourcePlacementCapture resourcePlacementCapture_;
  ResourcePlacementCaptureNoExecute resourcePlacementCaptureNoExecute_;
  ResourcePlacementPlayback resourcePlacementPlayback_;
  ResourcePlacementAssertions resourcePlacementAssertions_;
  bool storeResourcePlacementData_{};
  bool storeResourcePlacementDataNoExecute_{};
  bool useResourcePlacementData_{};
  bool portabilityChecks_{};
  bool portabilityAssertions_{};
  bool forcePlacedToCommittedResources_{};
  float accelerationStructurePadding_{1.0};
  float accelerationStructureScratchPadding_{1.0};
  std::unordered_set<unsigned> forcedCommittedResources_{};
};

} // namespace DirectX
} // namespace gits
