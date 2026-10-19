// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// The server's model of one instrumented process: what it told us about itself,
// which pipelines it has created, and the bytecode of their shaders. Purely a
// container - it neither talks to the network nor knows about the GUI.

namespace gits::rtsr {

// What the client said about itself during the handshake.
struct SessionInfo {
  std::string ApplicationPath;
  std::string ApplicationName;
  RTSR_API UsedApi{RTSR_API_UNKNOWN};
};

// One shader binding of one pipeline. The stage lives here rather than on the
// shader itself: identical bytecode can legitimately be bound at two different
// stages, and folding the stage into the deduplicated record is what makes
// RTSR's shader map report the wrong stage for the second one.
struct PipelineStage {
  RTSR_SHADER_STAGE Stage{RTSR_SHADER_STAGE_UNKNOWN};
  RTSR_HASH BytecodeHash{};
  std::string EntrypointName;
};

struct PipelineRecord {
  RTSR_ID Id{};
  RTSR_PIPELINE_TYPE Type{RTSR_PIPELINE_TYPE_UNKNOWN};
  // Empty until the client sends a name, which it may never do.
  std::string Name;
  RTSR_SERIALIZED_PIPELINE_STATE State;
  RTSR_HASH StateHash{};
  RTSR_PIPELINE_INCARNATION Incarnation{};
  bool ReplacementEligible{true};
  std::string ReplacementDiagnostic;
  std::vector<PipelineStage> Stages;
};

// Shader metadata without the bytecode, which is what a table row needs. The
// blob stays in the store rather than being copied out once per frame.
struct ShaderSummary {
  RTSR_HASH BytecodeHash{};
  RTSR_SHADER_STAGE Stage{RTSR_SHADER_STAGE_UNKNOWN};
  RTSR_BYTECODE_TYPE BytecodeType{RTSR_BYTECODE_TYPE_UNKNOWN};
  std::string EntrypointName;
  size_t D3DShaderVersion{};
  size_t BytecodeSize{};
};

// Every method is safe to call concurrently. Pipelines arrive on the transport's
// push thread, the session opens and closes on its request thread, and the GUI
// reads from the UI thread.
class SessionStore {
public:
  struct ShaderBlob {
    RTSR_BYTECODE_TYPE BytecodeType{RTSR_BYTECODE_TYPE_UNKNOWN};
    std::string EntrypointName;
    size_t D3DShaderVersion{};
    std::vector<uint8_t> Bytecode;
  };

  struct BoundShaderBlob {
    RTSR_SHADER_STAGE Stage{RTSR_SHADER_STAGE_UNKNOWN};
    RTSR_HASH BytecodeHash{};
    ShaderBlob Shader;
  };

  // A self-consistent, owned input for rebuilding one pipeline. Generation
  // identifies the live session it was copied from.
  struct ReplacementSnapshot {
    uint64_t Generation{};
    uint64_t PipelineRevision{};
    SessionInfo Session;
    PipelineRecord Pipeline;
    std::vector<BoundShaderBlob> Shaders;
  };

  // Replaces whatever was there. A client that reconnects starts from an empty
  // store, so two runs of an application can never appear merged, which is what
  // RTSR does when the previous client died without saying goodbye.
  void OpenSession(SessionInfo info);

  // Marks the session ended and keeps everything it harvested, so a finished
  // run stays on screen and stays attributable. The next OpenSession is what
  // replaces it.
  void CloseSession();

  // Discards the harvested data outright, for when the module itself is being
  // shut down rather than a run merely ending.
  void Clear();

  bool IsSessionOpen() const;
  SessionInfo Info() const;

  size_t PipelineCount() const;
  size_t ShaderCount() const;

  // Stores the pipeline and any shader bytecode not already known. Returns the
  // stage bindings whose bytecode was new, so the caller can announce them
  // without querying back.
  //
  // Takes the message by value: it carries every shader blob of one pipeline
  // state object, and a replay creates thousands of them.
  std::vector<PipelineStage> AddPipeline(RTSR_PIPELINE_CREATED_MESSAGE message);

  // Both return false for an unknown pipeline, which is normal: a name or a
  // release can arrive for a pipeline created before this session was opened.
  bool SetPipelineName(RTSR_ID id, std::string name);
  bool SetPipelineEligibility(const RTSR_PIPELINE_ELIGIBILITY_MESSAGE& message);
  bool RemovePipeline(RTSR_ID id);

  std::optional<PipelineRecord> Pipeline(RTSR_ID id) const;

  // One entry per stage of the pipeline, empty if it is unknown. The stage comes
  // from the pipeline's own binding, the rest from the shader record.
  std::vector<ShaderSummary> PipelineShaders(RTSR_ID id) const;

  // Copies under the lock so conversion can outlive a pipeline or session.
  std::optional<ShaderBlob> Shader(RTSR_HASH hash, std::string_view entrypointName) const;

  // Copies a pipeline and every shader blob it binds under one lock. Empty when
  // the session has ended, the pipeline is unknown, or one blob is unavailable.
  std::optional<ReplacementSnapshot> SnapshotForReplacement(RTSR_ID id) const;

  // Expensive work checks this before sending and before committing its result.
  bool IsCurrent(const ReplacementSnapshot& snapshot) const;

private:
  struct ShaderRecord {
    RTSR_BYTECODE_TYPE BytecodeType{RTSR_BYTECODE_TYPE_UNKNOWN};
    std::string EntrypointName;
    // Shader model as major * 10 + minor. Zero when the client could not
    // determine it, which is always the case on Vulkan.
    size_t D3DShaderVersion{};
    std::vector<uint8_t> Bytecode;
    // Number of pipeline stage bindings pointing here. The record is dropped
    // when the count reaches zero; RTSR keeps every blob it has ever seen for
    // the lifetime of the session.
    size_t ReferenceCount{};
  };

  // Both require the lock to be held.
  void ReleaseStagesLocked(const std::vector<PipelineStage>& stages);
  void ClearLocked();

  mutable std::mutex m_Mutex;
  bool m_SessionOpen{false};
  uint64_t m_Generation{};
  uint64_t m_NextPipelineRevision{};
  SessionInfo m_Info;
  std::unordered_map<RTSR_ID, PipelineRecord> m_Pipelines;
  std::unordered_map<RTSR_ID, uint64_t> m_PipelineRevisions;
  std::unordered_map<RTSR_HASH, ShaderRecord> m_Shaders;
};

} // namespace gits::rtsr
