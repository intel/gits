// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindingTablesDump.h"
#include "analyzerRaytracingService.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"

namespace gits {
namespace DirectX {

void BindingTablesDump::dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                         ID3D12Resource* resource,
                                         unsigned offset,
                                         unsigned size,
                                         unsigned stride,
                                         D3D12_RESOURCE_STATES state) {
  BindingTablesInfo* info = new BindingTablesInfo();
  info->offset = offset;
  info->size = size;
  info->stride = stride;

  stageResource(commandList, resource, state, *info);
}

void BindingTablesDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(mutex_);

  BindingTablesInfo& info = static_cast<BindingTablesInfo&>(dumpInfo);
  unsigned recordCount = info.size / info.stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * info.stride;
    unsigned count = (info.stride - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) / sizeof(UINT64);
    for (unsigned i = 0; i < count; ++i) {
      UINT64* address = reinterpret_cast<UINT64*>(p + sizeof(UINT64) * i);

      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          raytracingService_.getGpuAddressService().getResourceInfoByCaptureAddress(*address);
      if (resourceInfo) {
        bindingTablesResources_.insert(resourceInfo->key);
        continue;
      }

      unsigned stride{};
      CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo =
          raytracingService_.getDescriptorHandleService().getViewDescriptorHeapInfoByCaptureHandle(
              *address);
      stride = raytracingService_.getDescriptorHandleService().viewHeapIncrement();
      if (!heapInfo) {
        heapInfo = raytracingService_.getDescriptorHandleService()
                       .getSamplerDescriptorHeapInfoByCaptureHandle(*address);
        stride = raytracingService_.getDescriptorHandleService().samplerHeapIncrement();
      }
      if (heapInfo) {
        unsigned index{};
        index = (*address - heapInfo->captureStart) / stride;
        bindingTablesDescriptors_.insert({heapInfo->key, index});
        DescriptorState* state =
            raytracingService_.getDescriptorService().getDescriptorState(heapInfo->key, index);
        if (state) {
          bindingTablesResources_.insert(state->resourceKey);
        }
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
