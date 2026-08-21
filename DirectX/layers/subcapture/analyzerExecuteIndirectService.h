// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "capturePlayerGpuAddressService.h"
#include "analyzerRaytracingService.h"
#include "commandsAuto.h"
#include "analyzerExecuteIndirectDump.h"

namespace gits {
namespace DirectX {

class AnalyzerCommandListService;

class AnalyzerExecuteIndirectService {
public:
  AnalyzerExecuteIndirectService(ResourceStateTracker& resourceStateTracker,
                                 CapturePlayerGpuAddressService& gpuAddressService,
                                 AnalyzerRaytracingService& raytracingService,
                                 AnalyzerCommandListService& commandListService);
  ~AnalyzerExecuteIndirectService();
  AnalyzerExecuteIndirectService(AnalyzerExecuteIndirectService&) = delete;
  AnalyzerExecuteIndirectService& operator=(AnalyzerExecuteIndirectService&) = delete;

  void CreateCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& c);
  void ExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c);

  void Flush();
  void ExecuteCommandLists(CommandKey key,
                           GITSKey commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(CommandKey key,
                        GITSKey commandQueueKey,
                        GITSKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey key,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey key, GITSKey fenceKey, UINT64 fenceValue);

  std::unordered_set<GITSKey>& GetArgumentBuffersResources() {
    return m_ExecuteIndirectDump.GetArgumentBuffersResources();
  }

  CapturePlayerGpuAddressService& GetGpuAddressService() {
    return m_GpuAddressService;
  }

private:
  void LoadExecuteIndirectDispatchRays();

private:
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;
  AnalyzerRaytracingService& m_RaytracingService;
  AnalyzerCommandListService& m_CommandListService;
  AnalyzerExecuteIndirectDump m_ExecuteIndirectDump;

  std::unordered_map<CommandKey, D3D12_DISPATCH_RAYS_DESC> m_ExecuteIndirectDispatchRays;
  std::unordered_map<GITSKey, D3D12_COMMAND_SIGNATURE_DESC> m_CommandSignatures;
};

} // namespace DirectX
} // namespace gits
