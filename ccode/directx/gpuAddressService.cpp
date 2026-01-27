// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuAddressService.h"
#include "plog/Log.h"
#include <cassert>

#include <wrl/client.h>

namespace directx {

GpuAddressService& GpuAddressService::Get() {
  static GpuAddressService s_Instance;
  return s_Instance;
}

void GpuAddressService::CreateResource(unsigned resourceKey, ID3D12Resource* resource) {
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  D3D12_GPU_VIRTUAL_ADDRESS startAddress = resource->GetGPUVirtualAddress();
  assert(startAddress);

  m_StartAddressesByKey[resourceKey] = startAddress;
}

void GpuAddressService::CreatePlacedResource(unsigned resourceKey,
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

  auto itHeap = m_StartAddressesByKey.find(heapKey);
  assert(itHeap != m_StartAddressesByKey.end());
  D3D12_GPU_VIRTUAL_ADDRESS heapStartAddress = itHeap->second;

  D3D12_GPU_VIRTUAL_ADDRESS resourceStartAddress = heapStartAddress + heapOffset;
  m_StartAddressesByKey[resourceKey] = resourceStartAddress;
  m_PlacedResources.insert(resourceKey);
}

void GpuAddressService::CreateHeap(unsigned heapKey, ID3D12Heap* heap) {
  D3D12_HEAP_DESC desc = heap->GetDesc();
  if (desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  D3D12_GPU_VIRTUAL_ADDRESS startAddress = GetHeapGPUVirtualAddress(heap);
  m_StartAddressesByKey[heapKey] = startAddress;
}

D3D12_GPU_VIRTUAL_ADDRESS GpuAddressService::GetHeapGPUVirtualAddress(ID3D12Heap* heap) {
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
  assert(res == S_OK);

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
  assert(res == S_OK);

  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = dummyResource->GetGPUVirtualAddress();
  assert(gpuAddress);

  return gpuAddress;
}

D3D12_GPU_VIRTUAL_ADDRESS GpuAddressService::GetGpuAddress(unsigned resourceKey, unsigned offset) {
  if (!resourceKey) {
    return 0;
  }
  auto itResource = m_StartAddressesByKey.find(resourceKey);
  if (itResource != m_StartAddressesByKey.end()) {
    return itResource->second + offset;
  } else {
    auto itPlacedResource = m_ReleasedPlacedResources.find(resourceKey);
    assert(itPlacedResource != m_ReleasedPlacedResources.end());
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "CCode - Placed resource already released. Incorrect overlapping "
                     "resource used.";
      logged = true;
    }
    return itPlacedResource->second + offset;
  }
}

void GpuAddressService::DestroyInterface(unsigned interfaceKey) {
  if (m_PlacedResources.contains(interfaceKey)) {
    m_ReleasedPlacedResources[interfaceKey] = m_StartAddressesByKey[interfaceKey];
    m_PlacedResources.erase(interfaceKey);
  }

  m_StartAddressesByKey.erase(interfaceKey);
}

} // namespace directx
