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
      : stateService_(stateService) {}
  void storeBuffer(ID3D12GraphicsCommandList* commandList,
                   unsigned commandListKey,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   unsigned offset,
                   unsigned size,
                   D3D12_RESOURCE_STATES resourceState,
                   unsigned callKey,
                   bool isMappable,
                   unsigned uploadResourceKey);
  std::vector<BufferRestoreInfo>& getRestoreInfos(unsigned callKey) {
    return restoreCommands_[callKey];
  }
  void setDeviceKey(unsigned deviceKey) {
    deviceKey_ = deviceKey;
  }

protected:
  struct BufferInfo : public DumpInfo {
    unsigned resourceKey;
    unsigned callKey;
    unsigned commandListKey;
    bool isMappable;
    unsigned uploadResourceKey;
  };

  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  StateTrackingService& stateService_;
  std::unordered_map<unsigned, std::vector<BufferRestoreInfo>> restoreCommands_;
  unsigned deviceKey_{};
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
