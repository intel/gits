// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <mutex>
#include <unordered_map>

namespace gits {
namespace DirectX {

class AddressPinningStoreLayer : public Layer {
public:
  AddressPinningStoreLayer();
  ~AddressPinningStoreLayer();

  void post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void post(ID3D12Device4CreateReservedResource1Command& command) override;
  void post(ID3D12Device10CreateReservedResource2Command& command) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void post(ID3D12DeviceCreateHeapCommand& command) override;
  void post(ID3D12Device4CreateHeap1Command& command) override;
  void post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;
  void post(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;

private:
  void storeAddressRanges();

  template <typename CommandT>
  void handleResource(CommandT& command);

  template <typename CommandT>
  void handlePlacedResource(CommandT& command);

  template <typename CommandT>
  void handleHeap(CommandT& command);

  template <typename CommandT>
  void handleGetGPUVirtualAddress(CommandT& command);

private:
  std::mutex mutex_;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS_RANGE> resourceAddressRanges_;

  struct HeapAllocationInfo {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE addressRange;
    UINT64 alignment;
  };
  std::unordered_map<unsigned, HeapAllocationInfo> heapAddressRanges_;

  struct HeapInfo {
    unsigned heapKey;
    UINT64 offset;
  };
  std::unordered_map<unsigned, HeapInfo> heapInfoByPlacedResource_;
};

} // namespace DirectX
} // namespace gits
