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
    unsigned uploadResourceKey{};
    unsigned offset{};
    uint64_t bufferHash{};
    std::vector<CommandWriter*> restoreCommands;
  };

public:
  AccelerationStructuresBufferContentRestore(StateTrackingService& stateService)
      : stateService_(stateService), bufferHashCheck_(*this) {}
  void storeBuffer(ID3D12GraphicsCommandList* commandList,
                   unsigned commandListKey,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   unsigned offset,
                   unsigned size,
                   D3D12_RESOURCE_STATES resourceState,
                   unsigned buildCallKey,
                   bool isMappable,
                   unsigned uploadResourceKey);
  void setDeviceKey(unsigned deviceKey) {
    deviceKey_ = deviceKey;
  }
  std::unordered_map<unsigned, BufferRestoreInfo>& getRestoreInfos(unsigned buildCallKey) {
    return restoreInfos_[buildCallKey];
  }
  void removeRestoreInfos(unsigned buildCallKey);
  void setBuildStateId(unsigned buildCallKey, unsigned stateId);

protected:
  struct BufferInfo : public DumpInfo {
    unsigned resourceKey;
    unsigned buildCallKey;
    unsigned commandListKey;
    bool isMappable;
    unsigned uploadResourceKey;
  };

  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  StateTrackingService& stateService_;
  std::unordered_set<unsigned> buildCallKeysForRestore_;
  unsigned restoreInfoUniqueId_{};
  std::unordered_map<unsigned, std::unordered_map<unsigned, BufferRestoreInfo>> restoreInfos_;
  unsigned deviceKey_{};
  std::mutex mutex_;

  class BufferHashCheck {
  public:
    BufferHashCheck(AccelerationStructuresBufferContentRestore& contentRestore)
        : contentRestore_(contentRestore) {}
    void setBuildStateId(unsigned buildCallKey, unsigned stateId);
    bool dumpBuffer(unsigned buildCallKey,
                    unsigned restoreInfoId,
                    unsigned bufferKey,
                    unsigned bufferOffset,
                    uint64_t hash);

  private:
    AccelerationStructuresBufferContentRestore& contentRestore_;
    std::unordered_map<unsigned, unsigned> buildStateIdByCallKey_;
    std::unordered_map<unsigned, unsigned> buildCallKeyByStateId_;

    struct RestoreInfo {
      unsigned stateId;
      unsigned restoreId;
    };
    std::map<std::pair<unsigned, unsigned>, std::unordered_map<uint64_t, RestoreInfo>>
        bufferHashesByKeyOffset_;
  };
  BufferHashCheck bufferHashCheck_;
};

} // namespace DirectX
} // namespace gits
