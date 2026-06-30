// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace gits {
namespace vulkan {

// Loads the subcapture analysis file produced by a prior analysis pass and
// answers per-object "should this object be restored?" queries during the
// recording pass.  Mirrors the DirectX AnalyzerResults design.
//
// When optimization is disabled (Features.Subcapture.optimize == false) or the
// analysis file is empty / missing, RestoreObject() always returns true so the
// state-restore behaves exactly as the legacy single-pass flow (restore every
// live object).
class AnalyzerResults {
public:
  AnalyzerResults();

  // True if the given object key should be restored in the recording pass.
  // Returns true (restore everything) when optimization is off or no keys were
  // loaded; otherwise tests set membership.
  bool RestoreObject(uint64_t objectKey) const;

  // True if a previous analysis pass already wrote the analysis file.  Used by
  // the layer manager to choose between analysis and recording passes.
  static bool IsAnalysis();

  // Path of the analysis file derived from the stream directory and the
  // configured subcapture frame range (mirrors the DirectX naming pattern).
  static std::string GetAnalysisFileName();

private:
  bool m_Optimize{};
  std::unordered_set<uint64_t> m_ObjectKeys;
};

} // namespace vulkan
} // namespace gits
