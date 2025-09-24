// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBufferContentRestore.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "commandsCustom.h"
#include "commandWritersCustom.h"
#include "stateTrackingService.h"

namespace gits {
namespace DirectX {

void AccelerationStructuresBufferContentRestore::storeBuffer(ID3D12GraphicsCommandList* commandList,
                                                             ID3D12Resource* resource,
                                                             unsigned resourceKey,
                                                             unsigned offset,
                                                             unsigned size,
                                                             D3D12_RESOURCE_STATES resourceState,
                                                             unsigned buildCallKey,
                                                             bool isMappable) {
  BufferInfo* info = new BufferInfo();
  info->offset = offset;
  info->size = size;
  info->buildCallKey = buildCallKey;
  info->resourceKey = resourceKey;
  info->isMappable = isMappable;
  info->dumpName = L"BLAS build " + std::to_wstring(buildCallKey) + L" resource O" +
                   std::to_wstring(resourceKey);

  stageResource(commandList, resource, resourceState, *info);

  std::lock_guard<std::mutex> lock(mutex_);
  restoreBuilds_.insert(buildCallKey);
}

void AccelerationStructuresBufferContentRestore::dumpBuffer(DumpInfo& dumpInfo, void* data) {

  BufferInfo& info = static_cast<BufferInfo&>(dumpInfo);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (restoreBuilds_.find(info.buildCallKey) == restoreBuilds_.end()) {
      return;
    }
  }

  BufferRestoreInfo restoreInfo{};
  restoreInfo.bufferKey = info.resourceKey;
  restoreInfo.offset = info.offset;
  restoreInfo.isMappable = info.isMappable;
  restoreInfo.bufferHash = ComputeHash(data, info.size, THashType::XX);
  restoreInfo.bufferData = std::make_unique<std::vector<char>>(
      static_cast<char*>(data), static_cast<char*>(data) + info.size);

  std::lock_guard<std::mutex> lock(mutex_);
  restoreBuildInfos_[info.buildCallKey].push_back(std::move(restoreInfo));
}

void AccelerationStructuresBufferContentRestore::removeBuild(unsigned buildCallKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  restoreBuilds_.erase(buildCallKey);
  auto it = restoreBuildInfos_.find(buildCallKey);
  if (it != restoreBuildInfos_.end()) {
    restoreBuildInfos_.erase(buildCallKey);
  }
}

} // namespace DirectX
} // namespace gits
