// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

  std::unordered_set<unsigned>& GetArgumentBuffersResources() {
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

  std::unordered_map<unsigned, D3D12_DISPATCH_RAYS_DESC> m_ExecuteIndirectDispatchRays;
  std::unordered_map<unsigned, D3D12_COMMAND_SIGNATURE_DESC> m_CommandSignatures;
};

} // namespace DirectX
} // namespace gits
