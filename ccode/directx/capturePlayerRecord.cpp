// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerRecord.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "capturePlayerDescriptorHandleService.h"

namespace directx {

void RecordRelease(unsigned objectKey, ULONG refCountAfterRelease) {
  if (refCountAfterRelease != 0) {
    return;
  }
  CapturePlayerGpuAddressService::Get().DestroyInterface(objectKey);
  CapturePlayerDescriptorHandleService::Get().DestroyHeap(objectKey);
}

void RecordCreatePlacedResource(unsigned heapKey,
                                unsigned resourceKey,
                                D3D12_RESOURCE_FLAGS flags) {
  CapturePlayerGpuAddressService::Get().CreatePlacedResource(heapKey, resourceKey, flags);
}

D3D12_GPU_VIRTUAL_ADDRESS RecordGetGPUVirtualAddress(ID3D12Resource* resource,
                                                     unsigned resourceKey,
                                                     D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  const unsigned size = static_cast<unsigned>(desc.Width);
  CapturePlayerGpuAddressService::Get().AddGpuCaptureAddress(resource, resourceKey, size,
                                                             captureAddress);
  const D3D12_GPU_VIRTUAL_ADDRESS playerAddress = resource->GetGPUVirtualAddress();
  CapturePlayerGpuAddressService::Get().AddGpuPlayerAddress(resource, resourceKey, size,
                                                            playerAddress);
  return playerAddress;
}

D3D12_GPU_DESCRIPTOR_HANDLE RecordGetGPUDescriptorHandleForHeapStart(
    ID3D12DescriptorHeap* heap, unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE captureHandle) {
  CapturePlayerDescriptorHandleService::Get().AddCaptureHandle(heap, heapKey, captureHandle);
  const D3D12_GPU_DESCRIPTOR_HANDLE playerHandle = heap->GetGPUDescriptorHandleForHeapStart();
  CapturePlayerDescriptorHandleService::Get().AddPlayerHandle(heapKey, playerHandle);
  return playerHandle;
}

void* RecordGetShaderIdentifier(ID3D12StateObjectProperties* properties,
                                LPCWSTR exportName,
                                unsigned commandKey,
                                const uint8_t* captureIdentifier) {
  CapturePlayerShaderIdentifierService::ShaderIdentifier captureId{};
  memcpy(captureId.data(), captureIdentifier, captureId.size());
  CapturePlayerShaderIdentifierService::Get().AddCaptureShaderIdentifier(commandKey, captureId,
                                                                         exportName);

  void* playerIdentifier = properties->GetShaderIdentifier(exportName);

  CapturePlayerShaderIdentifierService::ShaderIdentifier playerId{};
  memcpy(playerId.data(), playerIdentifier, playerId.size());
  CapturePlayerShaderIdentifierService::Get().AddPlayerShaderIdentifier(commandKey, playerId,
                                                                        exportName);
  return playerIdentifier;
}

} // namespace directx
