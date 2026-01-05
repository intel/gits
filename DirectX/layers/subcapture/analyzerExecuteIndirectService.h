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

class AnalyzerExecuteIndirectService : gits::noncopyable {
public:
  AnalyzerExecuteIndirectService(CapturePlayerGpuAddressService& gpuAddressService,
                                 AnalyzerRaytracingService& raytracingService,
                                 AnalyzerCommandListService& commandListService);
  ~AnalyzerExecuteIndirectService();
  void createCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& c);
  void executeIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c);

  void flush();
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void commandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

  std::unordered_set<unsigned>& getArgumentBuffersResources() {
    return executeIndirectDump_.getArgumentBuffersResources();
  }

  CapturePlayerGpuAddressService& getGpuAddressService() {
    return gpuAddressService_;
  }

private:
  void loadExecuteIndirectDispatchRays();

private:
  CapturePlayerGpuAddressService& gpuAddressService_;
  AnalyzerRaytracingService& raytracingService_;
  AnalyzerCommandListService& commandListService_;
  AnalyzerExecuteIndirectDump executeIndirectDump_;

  std::unordered_map<unsigned, D3D12_DISPATCH_RAYS_DESC> executeIndirectDispatchRays_;
  std::unordered_map<unsigned, D3D12_COMMAND_SIGNATURE_DESC> commandSignatures_;
};

} // namespace DirectX
} // namespace gits
