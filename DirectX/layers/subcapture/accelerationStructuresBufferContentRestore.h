// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"
#include "commandWriter.h"

#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresBufferContentRestore : public ResourceDump {
public:
  struct BufferRestoreInfo {
    unsigned bufferKey{};
    unsigned offset{};
    uint64_t bufferHash{};
    bool isMappable{};
    std::unique_ptr<std::vector<char>> bufferData;
  };

public:
  AccelerationStructuresBufferContentRestore(StateTrackingService& stateService)
      : stateService_(stateService) {}
  void storeBuffer(ID3D12GraphicsCommandList* commandList,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   unsigned offset,
                   unsigned size,
                   D3D12_RESOURCE_STATES resourceState,
                   unsigned buildCallKey,
                   bool isMappable);
  std::vector<BufferRestoreInfo>& getRestoreInfos(unsigned buildCallKey) {
    return restoreBuildInfos_[buildCallKey];
  }
  void removeBuild(unsigned buildCallKey);
  void setDeviceKey(unsigned deviceKey) {
    deviceKey_ = deviceKey;
  }

protected:
  struct BufferInfo : public DumpInfo {
    unsigned resourceKey;
    unsigned buildCallKey;
    bool isMappable;
  };

  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  StateTrackingService& stateService_;
  std::unordered_set<unsigned> restoreBuilds_;
  std::unordered_map<unsigned, std::vector<BufferRestoreInfo>> restoreBuildInfos_;
  unsigned deviceKey_{};
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
