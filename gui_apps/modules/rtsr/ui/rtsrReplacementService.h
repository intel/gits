// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/server/rtsrServer.h"
#include "rtsr/shared/server/rtsrSessionStore.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace gits::gui {

class RtsrReplacementService {
public:
  using OperationId = uint64_t;

  enum class OperationKind {
    Apply,
    ResetPipeline
  };

  enum class OperationState {
    Pending,
    Succeeded,
    Failed
  };

  struct OperationResult {
    OperationId Id{};
    OperationKind Kind{OperationKind::Apply};
    OperationState State{OperationState::Pending};
    rtsr::RTSR_ID PipelineId{};
    rtsr::RTSR_SHADER_STAGE Stage{rtsr::RTSR_SHADER_STAGE_UNKNOWN};
    std::string Diagnostic;
  };

  struct ApplyRequest {
    rtsr::RTSR_ID PipelineId{};
    rtsr::RTSR_SHADER_STAGE Stage{rtsr::RTSR_SHADER_STAGE_UNKNOWN};
    rtsr::RTSR_HASH OriginalShaderHash{};
    size_t D3DShaderVersion{};
    std::string EntrypointName;
    std::string Source;
    std::filesystem::path PlayerPath;
  };

  RtsrReplacementService(const rtsr::SessionStore& store, rtsr::RtsrServer& server);
  ~RtsrReplacementService();

  RtsrReplacementService(const RtsrReplacementService&) = delete;
  RtsrReplacementService& operator=(const RtsrReplacementService&) = delete;

  OperationId Apply(ApplyRequest request);
  OperationId ResetPipeline(rtsr::RTSR_ID pipelineId);

  std::optional<OperationResult> Result(OperationId id) const;
  bool HasActiveReplacement(rtsr::RTSR_ID pipelineId) const;
  bool HasActiveReplacement(rtsr::RTSR_ID pipelineId, rtsr::RTSR_SHADER_STAGE stage) const;
  bool HasPendingOperation(rtsr::RTSR_ID pipelineId) const;

  // Invalidates queued/in-flight work and forgets runtime overlays. An
  // operation already inside the client is allowed to answer, but its stale
  // result cannot be committed.
  void Clear();
  void ForgetPipeline(rtsr::RTSR_ID pipelineId);

private:
  using StageOverlays =
      std::map<rtsr::RTSR_SHADER_STAGE, rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>;

  struct Work {
    OperationId Id{};
    OperationKind Kind{OperationKind::Apply};
    uint64_t ServiceGeneration{};
    uint64_t PipelineGeneration{};
    rtsr::SessionStore::ReplacementSnapshot Snapshot;
    ApplyRequest Apply;
  };

  OperationId Queue(OperationKind kind,
                    rtsr::RTSR_ID pipelineId,
                    rtsr::RTSR_SHADER_STAGE stage,
                    std::optional<rtsr::SessionStore::ReplacementSnapshot> snapshot,
                    ApplyRequest request = {});
  void Run();
  void ApplyShader(Work work);
  void Reset(Work work);
  void Finish(const Work& work, OperationState state, std::string diagnostic);
  bool WorkIsCurrent(const Work& work) const;

  std::optional<rtsr::RTSR_UPDATE_PIPELINES_REQUEST> BuildUpdateRequest(
      const Work& work, const StageOverlays& overlays, std::string& diagnostic) const;
  bool SnapshotIsCurrent(const rtsr::SessionStore::ReplacementSnapshot& snapshot) const;

  const rtsr::SessionStore& m_Store;
  rtsr::RtsrServer& m_Server;

  mutable std::mutex m_Mutex;
  std::condition_variable m_Wake;
  std::deque<Work> m_Queue;
  std::unordered_map<OperationId, OperationResult> m_Results;
  std::unordered_map<rtsr::RTSR_ID, StageOverlays> m_Overlays;
  std::unordered_map<rtsr::RTSR_ID, uint64_t> m_PipelineGenerations;
  OperationId m_NextOperation{1};
  uint64_t m_Generation{};
  bool m_Stopping{false};
  // Last: everything the worker can access must exist before it starts.
  std::thread m_Worker;
};

} // namespace gits::gui
