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

// Collects the set of objects actually referenced by commands inside the
// subcapture frame range during the analysis pass, expands that set into its
// full dependency closure (parents and sibling dependencies walked from the
// live ObjectState graph) and dumps it to the analysis file at range end.
//
// Mirrors the DirectX AnalyzerService.  Unlike DirectX, Vulkan2 already
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

  // Mark a single object key as used.  No-op outside the range, for zero keys,
  // or when optimization is disabled.
  void NotifyObject(uint64_t objectKey);

  // Mark a batch of object keys as used (e.g. a handle array / struct HandleKeys).
  void NotifyObjects(const std::vector<uint64_t>& objectKeys);

  // Compute the dependency closure of the collected roots and write the
  // analysis file.  Idempotent: only the first call writes.
  void DumpAnalysisFile();

private:
  // Recursively add key and everything it depends on to outKeys.
  void AddClosure(uint64_t key, std::set<uint64_t>& outKeys);

  StateTrackingService& m_StateTracking;
  SubcaptureRange& m_SubcaptureRange;
  bool m_Optimize{};
  bool m_Dumped{false};
  std::set<uint64_t> m_ObjectsForRestore;
};

} // namespace vulkan
} // namespace gits
