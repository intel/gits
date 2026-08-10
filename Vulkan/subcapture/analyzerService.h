// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRange.h"

#include <cstdint>
#include <set>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;
class AnalyzerRaytracingService;
class RaytracingOptimizationService;

// Collects the set of objects actually referenced by commands inside the
// subcapture frame range during the analysis pass, expands that set into its
// full dependency closure (parents and sibling dependencies walked from the
// live ObjectState graph) and dumps it to the analysis file at range end.
//
// Mirrors the DirectX AnalyzerService.  Unlike DirectX, Vulkan already
// maintains a complete per-object state graph (ObjectState::ParentKey /
// DependencyKeys plus a few type-specific links such as bound memory and
// descriptor/command pools) in the StateTrackingService, so the closure is
// computed by walking that graph rather than a separately-built parent map.
class AnalyzerService {
public:
  AnalyzerService(StateTrackingService& stateTracking, SubcaptureRange& subcaptureRange);
  ~AnalyzerService();
  AnalyzerService(const AnalyzerService&) = delete;
  AnalyzerService& operator=(const AnalyzerService&) = delete;

  // Add a single object key to the restore set. No-op outside the range, for
  // zero keys, or when optimization is disabled.
  void AddObjectForRestore(uint64_t objectKey);

  // Add a batch of object keys (e.g. a handle array / struct HandleKeys).
  void AddObjectsForRestore(const std::vector<uint64_t>& objectKeys);

  // Compute the dependency closure of the collected roots and write the
  // analysis file.  Idempotent: only the first call writes.
  void DumpAnalysisFile();

  // Optional: lets AddClosure pull a TLAS's referenced BLASes into the restore
  // closure. When unset, no BLAS is pulled in on a TLAS's behalf.
  void SetRaytracingService(AnalyzerRaytracingService* raytracingService) {
    m_RaytracingService = raytracingService;
  }

  // Optional: computes the retained BLAS ops for the analysis file's BlasChain section.
  void SetOptimizationService(RaytracingOptimizationService* optimizationService) {
    m_OptimizationService = optimizationService;
  }

private:
  // Recursively add key and everything it depends on to outKeys.
  void AddClosure(uint64_t key, std::set<uint64_t>& outKeys);

  // Add every live buffer whose device address the application queried, plus its
  // closure. A shader can reach such a buffer through the address alone, which the
  // handle-based closure walk cannot see.
  void AddDeviceAddressBufferClosure(std::set<uint64_t>& outKeys);

  StateTrackingService& m_StateTracking;
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerRaytracingService* m_RaytracingService{nullptr};
  RaytracingOptimizationService* m_OptimizationService{nullptr};
  bool m_Optimize{};
  bool m_Dumped{false};
  std::set<uint64_t> m_ObjectsForRestore;
};

} // namespace vulkan
} // namespace gits
