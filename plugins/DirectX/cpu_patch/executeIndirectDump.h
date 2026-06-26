// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configurationLib.h"
#include "resourceDump.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"
#include "bit_range.h"
#include "commandsAuto.h"
#include "commandsCustom.h"

#include <filesystem>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ExecuteIndirectDump : public ResourceDump {
public:
  ExecuteIndirectDump(const Configuration& gitsConfig,
                      ResourceStateTracker& resourceStateTracker,
                      CapturePlayerGpuAddressService& addressService,
                      std::unordered_map<unsigned, ID3D12Resource*>& resourceByKey);
  ~ExecuteIndirectDump();

  ExecuteIndirectDump(const ExecuteIndirectDump&) = delete;
  ExecuteIndirectDump& operator=(const ExecuteIndirectDump&) = delete;

  void executeIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

  void createCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& command);

protected:
  void dumpStagedResource(DumpInfo& dumpInfo);

private:
  struct ExecuteIndirectDumpInfo : DumpInfo {
    unsigned frameCount{};
    unsigned executeCount{};
  };

  void initialize();
  void loadExecuteIndirectDispatchRays();

  const Configuration& gitsConfig_;
  ResourceStateTracker& resourceStateTracker_;
  CapturePlayerGpuAddressService& addressService_;
  std::unordered_map<unsigned, ID3D12Resource*>& resourceByKey_;
  BitRange frames_;
  BitRange executions_;
  bool initialized_{false};
  std::filesystem::path dumpDir_;
  size_t numFiles_{};
  size_t filesTotalSize_{};

  std::unordered_map<unsigned, std::vector<D3D12_DISPATCH_RAYS_DESC>> executeIndirectDispatchRays_;
  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      commandSignatures_;
};

} // namespace DirectX
} // namespace gits
