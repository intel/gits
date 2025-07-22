// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "raytracingInstancesDump.h"
#include "capturePlayerGpuAddressService.h"

#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService {
public:
  AnalyzerRaytracingService() : instancesDump_(*this) {}
  void createStateObject(ID3D12Device5CreateStateObjectCommand& c);
  void buildTlas(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
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
  void captureGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c);
  void playerGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c);
  void genericReadResource(unsigned resourceKey) {
    genericReadResources_.insert(resourceKey);
  }

  CapturePlayerGpuAddressService& getGpuAddressService() {
    return gpuAddressService_;
  }

  std::vector<unsigned>& getStateObjectSubobjects(unsigned stateObjectKey) {
    return stateObjectsSubobjects_[stateObjectKey];
  }
  std::vector<std::pair<unsigned, unsigned>>& getBlasesForTlas(unsigned tlasBuildKey) {
    return blasesByTlas_[tlasBuildKey];
  }
  using BlasesByTlas = std::unordered_map<unsigned, std::vector<std::pair<unsigned, unsigned>>>;
  BlasesByTlas& getBlases() {
    return blasesByTlas_;
  }

private:
  CapturePlayerGpuAddressService gpuAddressService_;
  std::unordered_map<unsigned, std::vector<unsigned>> stateObjectsSubobjects_;
  RaytracingInstancesDump instancesDump_;
  BlasesByTlas blasesByTlas_;
  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;
  std::unordered_set<unsigned> genericReadResources_;
};

} // namespace DirectX
} // namespace gits
