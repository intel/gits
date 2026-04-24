// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(ID3D12DeviceCreateHeapCommand& command) override;
  void Post(ID3D12Device4CreateHeap1Command& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;

private:
  void StoreAddressRanges();

  template <typename CommandT>
  void HandleResource(CommandT& command);

  template <typename CommandT>
  void HandlePlacedResource(CommandT& command);

  template <typename CommandT>
  void HandleHeap(CommandT& command);

  template <typename CommandT>
  void HandleGetGPUVirtualAddress(CommandT& command);

private:
  std::mutex m_Mutex;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS_RANGE> m_ResourceAddressRanges;

  struct HeapAllocationInfo {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE m_AddressRange;
    UINT64 m_Alignment;
  };
  std::unordered_map<unsigned, HeapAllocationInfo> m_HeapAddressRanges;

  struct HeapInfo {
    unsigned m_HeapKey;
    UINT64 m_Offset;
  };
  std::unordered_map<unsigned, HeapInfo> m_HeapInfoByPlacedResource;
};

} // namespace DirectX
} // namespace gits
