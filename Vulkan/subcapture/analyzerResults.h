// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

// One retained BLAS operation loaded from the analysis file's BlasChain section, in
// replay (execution) order. Produced by RaytracingOptimizationService.
struct BlasChainOp {
  uint64_t CommandKey{};
  uint64_t DstAsKey{};         // destination AS (resolves device/queue for replay)
  uint64_t SourceCommandKey{}; // reduced-chain source op (0 for a root build)
  // Reduced-chain source AS (0 for a root build). An update must set the
  // reduced chain source, not the full chain source.
  uint64_t SrcAsKey{};
  bool IsCopy{};
  VkCopyAccelerationStructureModeKHR CopyMode{}; // valid only when IsCopy
};

// Loads the subcapture analysis file produced by a prior analysis pass and
// answers per-object "should this object be restored?" queries during the
// recording pass.  Mirrors the DirectX AnalyzerResults design.
//
// When optimization is disabled (Common.Player.Subcapture.optimize == false) or the
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

  // True if the analysis file carried a BlasChain section. Without one the recording
  // pass has no acceleration structure operations to replay and refuses a stream that
  // builds any before the range (see SubcaptureLayer::RequireBlasChainForRaytracing).
  bool HasBlasChain() const {
    return m_BlasChainLoaded;
  }

  bool CaptureAsBuildInputs() const {
    return m_CaptureAsBuildInputs;
  }

  // True if AS content is restored by replaying the retained build/update/copy chain
  // (portable). False means every AS is restored from a serialized blob instead.
  bool UseAsChainRestore() const {
    return m_Optimize && m_CaptureAsBuildInputs && m_BlasChainLoaded;
  }

  // True if this BLAS op is part of the retained reduced chain, or if there is no
  // chain to consult (optimization off / no BlasChain loaded), matching the legacy
  // restore-everything flow.
  bool RestoreBlasCommand(uint64_t commandKey) const;

  // Strict membership in the retained reduced chain, unlike RestoreBlasCommand. Use
  // it wherever being retained decides whether a failure is fatal.
  bool IsRetainedBlasCommand(uint64_t commandKey) const {
    return m_RetainedBlasCommands.find(commandKey) != m_RetainedBlasCommands.end();
  }

  // Reduced-chain source op for a retained update (0 for a root build or an
  // unknown/uncollapsed command).
  uint64_t GetBlasSourceCommand(uint64_t commandKey) const;

  // The retained BLAS chain in replay order (empty when none loaded).
  const std::vector<BlasChainOp>& GetBlasChain() const {
    return m_BlasChain;
  }

  // True if a previous analysis pass already wrote the analysis file.  Used by
  // the layer manager to choose between analysis and recording passes.
  static bool IsAnalysis();

  // Path of the analysis file derived from the stream directory and the
  // configured subcapture frame range (mirrors the DirectX naming pattern).
  static std::string GetAnalysisFileName();

private:
  // True if the analysis file exists, parses as YAML and carries the trailing
  // completion marker written by a fully finished analysis pass.  A missing,
  // truncated or unparsable file (e.g. from an interrupted analysis run) returns
  // false so it is treated as "no valid analysis".
  static bool IsCompleteAnalysisFile(const std::string& fileName);

  bool m_Optimize{};
  bool m_CaptureAsBuildInputs{};
  std::unordered_set<uint64_t> m_ObjectKeys;
  bool m_BlasChainLoaded{false};
  std::vector<BlasChainOp> m_BlasChain;
  std::unordered_set<uint64_t> m_RetainedBlasCommands;
  std::unordered_map<uint64_t, uint64_t> m_BlasSourceByCommand;
};

} // namespace vulkan
} // namespace gits
