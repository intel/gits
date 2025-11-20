// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerGpuAddressService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void PlayerGpuAddressService::createResource(unsigned resourceKey, ID3D12Resource* resource) {

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  D3D12_GPU_VIRTUAL_ADDRESS startAddress = resource->GetGPUVirtualAddress();
  GITS_ASSERT(startAddress);

  startAddressesByKey_[resourceKey] = startAddress;
}

void PlayerGpuAddressService::createPlacedResource(unsigned resourceKey,
                                                   ID3D12Resource* resource,
                                                   unsigned heapKey,
                                                   ID3D12Heap* heap,
                                                   UINT64 heapOffset) {

  D3D12_HEAP_DESC heapDesc = heap->GetDesc();
  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  auto itHeap = startAddressesByKey_.find(heapKey);
  GITS_ASSERT(itHeap != startAddressesByKey_.end());
  D3D12_GPU_VIRTUAL_ADDRESS heapStartAddress = itHeap->second;

  D3D12_GPU_VIRTUAL_ADDRESS resourceStartAddress = heapStartAddress + heapOffset;
  startAddressesByKey_[resourceKey] = resourceStartAddress;
  placedResources_.insert(resourceKey);
}

void PlayerGpuAddressService::createHeap(unsigned heapKey, ID3D12Heap* heap) {

  D3D12_HEAP_DESC desc = heap->GetDesc();
  if (desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  D3D12_GPU_VIRTUAL_ADDRESS startAddress = getHeapGPUVirtualAddress(heap);
  startAddressesByKey_[heapKey] = startAddress;
}

D3D12_GPU_VIRTUAL_ADDRESS PlayerGpuAddressService::getHeapGPUVirtualAddress(ID3D12Heap* heap) {

  D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC dummyDesc{};
  dummyDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  dummyDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  dummyDesc.Width = 1;
  dummyDesc.Height = 1;
  dummyDesc.DepthOrArraySize = 1;
  dummyDesc.MipLevels = 1;
  dummyDesc.Format = DXGI_FORMAT_UNKNOWN;
  dummyDesc.SampleDesc = {1, 0};
  dummyDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  dummyDesc.Flags = heapDesc.Flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER
                        ? D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER
                        : D3D12_RESOURCE_FLAG_NONE;

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT res = heap->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(res == S_OK);

  D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD ||
      heapDesc.Properties.Type == D3D12_HEAP_TYPE_GPU_UPLOAD) {
    initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
  } else if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK) {
    initialState = D3D12_RESOURCE_STATE_COPY_DEST;
  }

  Microsoft::WRL::ComPtr<ID3D12Resource> dummyResource;
  res = device->CreatePlacedResource(heap, 0, &dummyDesc, initialState, nullptr,
                                     IID_PPV_ARGS(&dummyResource));
  GITS_ASSERT(res == S_OK);

  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = dummyResource->GetGPUVirtualAddress();
  GITS_ASSERT(gpuAddress);

  return gpuAddress;
}

D3D12_GPU_VIRTUAL_ADDRESS PlayerGpuAddressService::getGpuAddress(unsigned resourceKey,
                                                                 unsigned offset) {
  if (!resourceKey) {
    return 0;
  }
  auto itResource = startAddressesByKey_.find(resourceKey);
  if (itResource != startAddressesByKey_.end()) {
    return itResource->second + offset;
  } else {
    auto itPlacedResouce = releasedPlacedResources_.find(resourceKey);
    GITS_ASSERT(itPlacedResouce != releasedPlacedResources_.end());
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "PlayerGpuAddressService - placed resource already released. Incorrect "
                     "overlapping resource used.";
      logged = true;
    }
    return itPlacedResouce->second + offset;
  }
}

void PlayerGpuAddressService::destroyInterface(unsigned interfaceKey) {

  if (placedResources_.contains(interfaceKey)) {
    releasedPlacedResources_[interfaceKey] = startAddressesByKey_[interfaceKey];
    placedResources_.erase(interfaceKey);
  }

  startAddressesByKey_.erase(interfaceKey);
}

} // namespace DirectX
} // namespace gits
