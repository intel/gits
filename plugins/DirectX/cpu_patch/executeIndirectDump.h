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

  void ExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

  void CreateCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& command);

protected:
  void DumpStagedResource(DumpInfo& dumpInfo);

private:
  struct ExecuteIndirectDumpInfo : DumpInfo {
    unsigned FrameCount{};
    unsigned ExecuteCount{};
  };

  void Initialize();
  void LoadExecuteIndirectDispatchRays();

  const Configuration& m_GitsConfig;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_AddressService;
  std::unordered_map<unsigned, ID3D12Resource*>& m_ResourceByKey;
  BitRange m_Frames;
  BitRange m_Executions;
  bool m_Initialized{false};
  std::filesystem::path m_DumpDir;
  size_t m_NumFiles{};
  size_t m_FilesTotalSize{};

  std::unordered_map<unsigned, std::vector<D3D12_DISPATCH_RAYS_DESC>> m_ExecuteIndirectDispatchRays;
  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      m_CommandSignatures;
};

} // namespace DirectX
} // namespace gits
