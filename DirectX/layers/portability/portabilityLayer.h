// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "heapPlacementOrganizer.h"

namespace gits {
namespace DirectX {

class PortabilityLayer : public Layer {

public:
  PortabilityLayer();
  ~PortabilityLayer();

public:
  void post(D3D12CreateDeviceCommand& command) override;

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

  void pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;

private:
  void checkHeapCreationFlags(unsigned heapKey,
                              ID3D12Device* device,
                              D3D12_HEAP_DESC* desc,
                              HRESULT result);

private:
  HeapPlacementOrganizer heapPlacementOrganizer_;
  bool isPlayer_;
};

} // namespace DirectX
} // namespace gits
