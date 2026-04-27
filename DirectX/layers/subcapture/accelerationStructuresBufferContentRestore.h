// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"
#include "commandSerializer.h"

#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresBufferContentRestore : public ResourceDump {
public:
  struct BufferRestoreInfo {
    unsigned BufferKey{};
    unsigned Offset{};
    unsigned BufferHash{};
    bool IsMappable{};
    std::unique_ptr<std::vector<char>> BufferData;
  };

public:
  AccelerationStructuresBufferContentRestore(StateTrackingService& stateService)
      : m_StateService(stateService) {}
  void StoreBuffer(ID3D12GraphicsCommandList* commandList,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   unsigned offset,
                   unsigned size,
                   BarrierState resourceState,
                   unsigned buildCallKey,
                   bool isMappable);
  std::vector<BufferRestoreInfo>& GetRestoreInfos(unsigned buildCallKey) {
    return m_RestoreBuildInfos[buildCallKey];
  }
  void RemoveBuild(unsigned buildCallKey);
  void SetDeviceKey(unsigned deviceKey) {
    m_DeviceKey = deviceKey;
  }

protected:
  struct BufferInfo : public DumpInfo {
    unsigned ResourceKey;
    unsigned BuildCallKey;
    bool IsMappable;
  };

  void DumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  StateTrackingService& m_StateService;
  std::unordered_set<unsigned> m_RestoreBuilds;
  std::unordered_map<unsigned, std::vector<BufferRestoreInfo>> m_RestoreBuildInfos;
  unsigned m_DeviceKey{};
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
