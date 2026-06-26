// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

namespace directx {

void RecordRelease(unsigned objectKey, ULONG refCountAfterRelease);
void RecordCreatePlacedResource(unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags);
D3D12_GPU_VIRTUAL_ADDRESS RecordGetGPUVirtualAddress(ID3D12Resource* resource,
                                                     unsigned resourceKey,
                                                     D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
D3D12_GPU_DESCRIPTOR_HANDLE RecordGetGPUDescriptorHandleForHeapStart(
    ID3D12DescriptorHeap* heap, unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE captureHandle);
void* RecordGetShaderIdentifier(ID3D12StateObjectProperties* properties,
                                LPCWSTR exportName,
                                unsigned commandKey,
                                const uint8_t* captureIdentifier);

} // namespace directx
