// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include <wrl/client.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace DirectX {

class AddressPinningUseLayer : public Layer {
public:
  AddressPinningUseLayer();

  void pre(D3D12CreateDeviceCommand& command) override;
  void post(D3D12CreateDeviceCommand& command) override;
  void pre(ID3D12DeviceCreateHeapCommand& command) override;
  void post(ID3D12DeviceCreateHeapCommand& command) override;
  void pre(ID3D12Device4CreateHeap1Command& command) override;
  void post(ID3D12Device4CreateHeap1Command& command) override;
  void pre(CreateHeapAllocationMetaCommand& command) override;
  void pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void pre(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void pre(ID3D12Device4CreateCommittedResource1Command& command) override;
  void post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void pre(ID3D12Device8CreateCommittedResource2Command& command) override;
  void post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void pre(ID3D12Device10CreateCommittedResource3Command& command) override;
  void post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void pre(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void pre(ID3D12Device4CreateReservedResource1Command& command) override;
  void post(ID3D12Device4CreateReservedResource1Command& command) override;
  void pre(ID3D12Device10CreateReservedResource2Command& command) override;
  void post(ID3D12Device10CreateReservedResource2Command& command) override;
  void pre(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void pre(ID3D12Device8CreatePlacedResource1Command& command) override;
  void post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void pre(ID3D12Device10CreatePlacedResource2Command& command) override;
  void post(ID3D12Device10CreatePlacedResource2Command& command) override;

private:
  void readAddressRanges();

  template <typename CommandT>
  void preResource(CommandT& command);

  template <typename CommandT>
  void postResource(CommandT& command);

  template <typename CommandT>
  void preHeap(CommandT& command);

  template <typename CommandT>
  void postHeap(CommandT& command);

  template <typename CommandT>
  void preOpenExistingHeap(CommandT& command);

private:
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS_RANGE> addressRanges_;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS_RANGE> resourceAddressRanges_;
  struct HeapAllocationInfo {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE addressRange;
    UINT64 alignment;
  };
  std::unordered_map<unsigned, HeapAllocationInfo> heapAddressRanges_;
  std::unordered_set<unsigned> changedHeaps_;
  Microsoft::WRL::ComPtr<ID3D12Tools1> d3d12Tools_;
  Microsoft::WRL::ComPtr<ID3D12DeviceTools> deviceTools_;
};

} // namespace DirectX
} // namespace gits
