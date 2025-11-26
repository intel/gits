// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

#include <set>

namespace gits {
namespace DirectX {

class PortabilityLayer : public Layer {

public:
  PortabilityLayer();
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
  void post(ID3D12DeviceGetResourceAllocationInfoCommand& c) override;
  void pre(ID3D12Device4GetResourceAllocationInfo1Command& c) override;
  void post(ID3D12Device4GetResourceAllocationInfo1Command& c) override;
  void pre(ID3D12Device8GetResourceAllocationInfo2Command& c) override;
  void post(ID3D12Device8GetResourceAllocationInfo2Command& c) override;
  void pre(ID3D12Device12GetResourceAllocationInfo3Command& c) override;
  void post(ID3D12Device12GetResourceAllocationInfo3Command& c) override;

private:
  void configureHeapMemoryPool(ID3D12Device* device, D3D12_HEAP_DESC* heapDesc);
  void checkHeapCreationFlags(unsigned heapKey, ID3D12Device* device, D3D12_HEAP_DESC* desc);

private:
  ResourcePlacementCapture resourcePlacementCapture_;
  ResourcePlacementCaptureNoExecute resourcePlacementCaptureNoExecute_;
  ResourcePlacementPlayback resourcePlacementPlayback_;
  ResourcePlacementAssertions resourcePlacementAssertions_;
  bool storeResourcePlacementData_{};
  bool storeResourcePlacementDataNoExecute_{};
  bool useResourcePlacementData_{};
  bool portabilityChecks_{};
  bool portabilityAssertions_{};
  float accelerationStructurePadding_{};
  float accelerationStructureScratchPadding_{};
};

} // namespace DirectX
} // namespace gits
