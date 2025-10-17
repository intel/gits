// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "capturePlayerGpuAddressService.h"
#include "analyzerRaytracingService.h"
#include "commandsAuto.h"

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

private:
  void loadExecuteIndirectDispatchRays();

private:
  CapturePlayerGpuAddressService& gpuAddressService_;
  AnalyzerRaytracingService& raytracingService_;
  AnalyzerCommandListService& commandListService_;
  std::unordered_map<unsigned, D3D12_DISPATCH_RAYS_DESC> executeIndirectDispatchRays_;
  std::unordered_map<unsigned, D3D12_COMMAND_SIGNATURE_DESC> commandSignatures_;
};

} // namespace DirectX
} // namespace gits
