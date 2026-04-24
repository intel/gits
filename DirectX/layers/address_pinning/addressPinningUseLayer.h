// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

  void Pre(D3D12CreateDeviceCommand& command) override;
  void Post(D3D12CreateDeviceCommand& command) override;
  void Pre(ID3D12DeviceCreateHeapCommand& command) override;
  void Post(ID3D12DeviceCreateHeapCommand& command) override;
  void Pre(ID3D12Device4CreateHeap1Command& command) override;
  void Post(ID3D12Device4CreateHeap1Command& command) override;
  void Pre(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Pre(CreateHeapAllocationMetaCommand& command) override;
  void Pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Pre(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Pre(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Pre(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Pre(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Pre(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Pre(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Pre(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Pre(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Pre(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Pre(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;

private:
  void ReadAddressRanges();

  template <typename CommandT>
  void PreResource(CommandT& command);

  template <typename CommandT>
  void PostResource(CommandT& command);

  template <typename CommandT>
  void PreHeap(CommandT& command);

  template <typename CommandT>
  void PostHeap(CommandT& command);

  template <typename CommandT>
  void PreOpenExistingHeap(CommandT& command);

private:
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS_RANGE> m_AddressRanges;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS_RANGE> m_ResourceAddressRanges;
  struct HeapAllocationInfo {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE m_AddressRange;
    UINT64 m_Alignment;
  };
  std::unordered_map<unsigned, HeapAllocationInfo> m_HeapAddressRanges;
  std::unordered_set<unsigned> m_ChangedHeaps;
  Microsoft::WRL::ComPtr<ID3D12Tools1> m_D3d12Tools;
  Microsoft::WRL::ComPtr<ID3D12DeviceTools> m_DeviceTools;
};

} // namespace DirectX
} // namespace gits
