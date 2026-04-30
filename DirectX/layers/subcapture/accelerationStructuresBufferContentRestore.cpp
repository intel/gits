// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBufferContentRestore.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandsCustom.h"
#include "commandSerializersCustom.h"
#include "stateTrackingService.h"

#include "xxhash.h"

namespace gits {
namespace DirectX {

void AccelerationStructuresBufferContentRestore::StoreBuffer(ID3D12GraphicsCommandList* commandList,
                                                             ID3D12Resource* resource,
                                                             unsigned ResourceKey,
                                                             unsigned offset,
                                                             unsigned size,
                                                             BarrierState resourceState,
                                                             unsigned buildCallKey,
                                                             bool isMappable) {
  BufferInfo* info = new BufferInfo();
  info->Offset = offset;
  info->Size = size;
  info->BuildCallKey = buildCallKey;
  info->ResourceKey = ResourceKey;
  info->IsMappable = isMappable;
  info->DumpName = L"BLAS build " + std::to_wstring(buildCallKey) + L" resource O" +
                   std::to_wstring(ResourceKey);

  StageResource(commandList, resource, resourceState, *info);

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_RestoreBuilds.insert(buildCallKey);
}

void AccelerationStructuresBufferContentRestore::DumpBuffer(DumpInfo& dumpInfo, void* data) {

  BufferInfo& info = static_cast<BufferInfo&>(dumpInfo);
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_RestoreBuilds.find(info.BuildCallKey) == m_RestoreBuilds.end()) {
      return;
    }
  }

  BufferRestoreInfo restoreInfo{};
  restoreInfo.BufferKey = info.ResourceKey;
  restoreInfo.Offset = info.Offset;
  restoreInfo.IsMappable = info.IsMappable;
  restoreInfo.BufferHash = XXH32(data, info.Size, 0);
  restoreInfo.BufferData = std::make_unique<std::vector<char>>(
      static_cast<char*>(data), static_cast<char*>(data) + info.Size);

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_RestoreBuildInfos[info.BuildCallKey].push_back(std::move(restoreInfo));
}

void AccelerationStructuresBufferContentRestore::RemoveBuild(unsigned buildCallKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_RestoreBuilds.erase(buildCallKey);
  auto it = m_RestoreBuildInfos.find(buildCallKey);
  if (it != m_RestoreBuildInfos.end()) {
    m_RestoreBuildInfos.erase(buildCallKey);
  }
}

} // namespace DirectX
} // namespace gits
