// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrReplacementService.h"

#include "rtsr/shared/rtsrCommon.h"
#include "rtsr/toolchain/rtsrShaderToolchain.h"
#include "rtsr/ui/rtsrEvents.h"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

namespace gits::gui {
namespace {

void PublishReplacementUpdate() {
  RtsrEvent event;
  event.EventType = RtsrEvent::Type::ReplacementUpdated;
  EventBus::GetInstance().publish<RtsrEvent>(event);
}

bool IsStageValidForPipeline(rtsr::RTSR_PIPELINE_TYPE pipelineType, rtsr::RTSR_SHADER_STAGE stage) {
  if (pipelineType == rtsr::RTSR_PIPELINE_TYPE_COMPUTE) {
    return stage == rtsr::RTSR_SHADER_STAGE_COMPUTE;
  }
  if (pipelineType == rtsr::RTSR_PIPELINE_TYPE_GRAPHICS) {
    return stage >= rtsr::RTSR_SHADER_STAGE_VERTEX && stage <= rtsr::RTSR_SHADER_STAGE_PIXEL;
  }
  return false;
}

void AppendDiagnostic(std::string& destination, const std::string& message) {
  if (message.empty()) {
    return;
  }
  if (!destination.empty()) {
    destination += '\n';
  }
  destination += message;
}

} // namespace

RtsrReplacementService::RtsrReplacementService(const rtsr::SessionStore& store,
                                               rtsr::RtsrServer& server)
    : m_Store(store), m_Server(server), m_Worker([this] { Run(); }) {}

RtsrReplacementService::~RtsrReplacementService() {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Stopping = true;
    m_Queue.clear();
  }
  m_Wake.notify_one();
  m_Worker.join();
}

RtsrReplacementService::OperationId RtsrReplacementService::Queue(
    OperationKind kind,
    rtsr::RTSR_ID pipelineId,
    rtsr::RTSR_SHADER_STAGE stage,
    std::optional<rtsr::SessionStore::ReplacementSnapshot> snapshot,
    ApplyRequest request) {
  OperationId id{};
  bool queued = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    id = m_NextOperation++;
    OperationResult result{id, kind, OperationState::Pending, pipelineId, stage, {}};
    if (m_Stopping || !snapshot.has_value()) {
      result.State = OperationState::Failed;
      result.Diagnostic = "The pipeline is not available in a live RTSR session.";
    } else {
      m_Queue.push_back({id, kind, m_Generation, m_PipelineGenerations[pipelineId],
                         std::move(*snapshot), std::move(request)});
      queued = true;
    }
    m_Results.emplace(id, std::move(result));
  }

  if (queued) {
    m_Wake.notify_one();
  } else {
    PublishReplacementUpdate();
  }
  return id;
}

RtsrReplacementService::OperationId RtsrReplacementService::Apply(ApplyRequest request) {
  const auto pipelineId = request.PipelineId;
  const auto stage = request.Stage;
  return Queue(OperationKind::Apply, pipelineId, stage, m_Store.SnapshotForReplacement(pipelineId),
               std::move(request));
}

RtsrReplacementService::OperationId RtsrReplacementService::ResetPipeline(
    rtsr::RTSR_ID pipelineId) {
  return Queue(OperationKind::ResetPipeline, pipelineId, rtsr::RTSR_SHADER_STAGE_UNKNOWN,
               m_Store.SnapshotForReplacement(pipelineId));
}

std::optional<RtsrReplacementService::OperationResult> RtsrReplacementService::Result(
    OperationId id) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto result = m_Results.find(id);
  return result == m_Results.end() ? std::nullopt : std::optional<OperationResult>(result->second);
}

bool RtsrReplacementService::HasActiveReplacement(rtsr::RTSR_ID pipelineId) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto pipeline = m_Overlays.find(pipelineId);
  return pipeline != m_Overlays.end() && !pipeline->second.empty();
}

bool RtsrReplacementService::HasActiveReplacement(rtsr::RTSR_ID pipelineId,
                                                  rtsr::RTSR_SHADER_STAGE stage) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto pipeline = m_Overlays.find(pipelineId);
  return pipeline != m_Overlays.end() && pipeline->second.find(stage) != pipeline->second.end();
}

bool RtsrReplacementService::HasPendingOperation(rtsr::RTSR_ID pipelineId) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return std::any_of(m_Results.begin(), m_Results.end(), [pipelineId](const auto& entry) {
    return entry.second.PipelineId == pipelineId && entry.second.State == OperationState::Pending;
  });
}

void RtsrReplacementService::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  ++m_Generation;
  m_Queue.clear();
  m_Results.clear();
  m_Overlays.clear();
  m_PipelineGenerations.clear();
}

void RtsrReplacementService::ForgetPipeline(rtsr::RTSR_ID pipelineId) {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    ++m_PipelineGenerations[pipelineId];
    m_Overlays.erase(pipelineId);
    std::erase_if(m_Queue, [pipelineId](const Work& work) {
      return work.Snapshot.Pipeline.Id == pipelineId;
    });
    for (auto& entry : m_Results) {
      if (entry.second.PipelineId == pipelineId && entry.second.State == OperationState::Pending) {
        entry.second.State = OperationState::Failed;
        entry.second.Diagnostic = "The pipeline was released or recreated.";
        changed = true;
      }
    }
  }
  if (changed) {
    PublishReplacementUpdate();
  }
}

bool RtsrReplacementService::WorkIsCurrent(const Work& work) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto pipeline = m_PipelineGenerations.find(work.Snapshot.Pipeline.Id);
  const uint64_t generation = pipeline == m_PipelineGenerations.end() ? 0 : pipeline->second;
  return !m_Stopping && work.ServiceGeneration == m_Generation &&
         work.PipelineGeneration == generation;
}

bool RtsrReplacementService::SnapshotIsCurrent(
    const rtsr::SessionStore::ReplacementSnapshot& snapshot) const {
  return m_Store.IsCurrent(snapshot);
}

std::optional<rtsr::RTSR_UPDATE_PIPELINES_REQUEST> RtsrReplacementService::BuildUpdateRequest(
    const Work& work, const StageOverlays& overlays, std::string& diagnostic) const {
  const auto& snapshot = work.Snapshot;
  if (snapshot.Pipeline.State.empty() || snapshot.Shaders.empty()) {
    diagnostic = "The captured pipeline state or its shaders are incomplete.";
    return std::nullopt;
  }

  rtsr::RTSR_UPDATE_PIPELINES_REQUEST request;
  rtsr::RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO pipeline;
  pipeline.Id = snapshot.Pipeline.Id;
  pipeline.Type = snapshot.Pipeline.Type;
  pipeline.State = snapshot.Pipeline.State;
  pipeline.StateHash = snapshot.Pipeline.StateHash;
  pipeline.Incarnation = snapshot.Pipeline.Incarnation;

  using StageIdentity = std::pair<rtsr::RTSR_HASH, rtsr::RTSR_SHADER_STAGE>;
  std::map<StageIdentity, uint32_t> references;
  std::set<rtsr::RTSR_SHADER_STAGE> stages;

  for (const auto& bound : snapshot.Shaders) {
    if (!IsStageValidForPipeline(snapshot.Pipeline.Type, bound.Stage) ||
        !stages.insert(bound.Stage).second) {
      diagnostic = "The pipeline has an invalid or duplicate shader stage.";
      return std::nullopt;
    }

    rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO shader;
    const auto replacement = overlays.find(bound.Stage);
    if (replacement != overlays.end()) {
      shader = replacement->second;
    } else {
      shader.ShaderStage = bound.Stage;
      shader.Bytecode = bound.Shader.Bytecode;
      shader.BytecodeHash = bound.BytecodeHash;
      shader.EntrypointName = bound.Shader.EntrypointName;
      shader.BytecodeType = bound.Shader.BytecodeType;
      shader.D3DShaderVersion = bound.Shader.D3DShaderVersion;
    }

    if (shader.ShaderStage != bound.Stage || shader.Bytecode.empty()) {
      diagnostic = "A shader reference has no stage-appropriate bytecode.";
      return std::nullopt;
    }

    const StageIdentity identity{shader.BytecodeHash, shader.ShaderStage};
    auto reference = references.find(identity);
    if (reference == references.end()) {
      if (request.ShaderStages.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        diagnostic = "The pipeline has too many shader stages.";
        return std::nullopt;
      }
      const uint32_t index = static_cast<uint32_t>(request.ShaderStages.size());
      request.ShaderStages.push_back(std::move(shader));
      reference = references.emplace(identity, index).first;
    }
    pipeline.ShaderReferences.push_back(reference->second);
  }

  if (pipeline.ShaderReferences.size() != snapshot.Shaders.size()) {
    diagnostic = "The update does not reference every shader bound by the pipeline.";
    return std::nullopt;
  }

  request.UpdatedPipelines.push_back(std::move(pipeline));
  return request;
}

void RtsrReplacementService::Finish(const Work& work,
                                    OperationState state,
                                    std::string diagnostic) {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto pipeline = m_PipelineGenerations.find(work.Snapshot.Pipeline.Id);
    const uint64_t pipelineGeneration =
        pipeline == m_PipelineGenerations.end() ? 0 : pipeline->second;
    if (m_Stopping || work.ServiceGeneration != m_Generation ||
        work.PipelineGeneration != pipelineGeneration) {
      return;
    }
    const auto result = m_Results.find(work.Id);
    if (result == m_Results.end()) {
      return;
    }
    result->second.State = state;
    result->second.Diagnostic = std::move(diagnostic);
  }
  PublishReplacementUpdate();
}

void RtsrReplacementService::ApplyShader(Work work) {
  const auto original = std::find_if(work.Snapshot.Shaders.begin(), work.Snapshot.Shaders.end(),
                                     [&work](const auto& shader) {
                                       return shader.Stage == work.Apply.Stage &&
                                              shader.BytecodeHash == work.Apply.OriginalShaderHash;
                                     });
  if (original == work.Snapshot.Shaders.end()) {
    Finish(work, OperationState::Failed,
           "The selected shader is no longer bound by this pipeline.");
    return;
  }
  const bool directX = work.Snapshot.Session.UsedApi == rtsr::RTSR_API_DX12 &&
                       original->Shader.BytecodeType == rtsr::RTSR_BYTECODE_TYPE_DXIL &&
                       original->Shader.D3DShaderVersion == work.Apply.D3DShaderVersion;
  const bool vulkan = work.Snapshot.Session.UsedApi >= rtsr::RTSR_API_VULKAN_1_0 &&
                      work.Snapshot.Session.UsedApi <= rtsr::RTSR_API_VULKAN_1_3 &&
                      original->Shader.BytecodeType == rtsr::RTSR_BYTECODE_TYPE_SPIRV;
  if (!directX && !vulkan) {
    Finish(work, OperationState::Failed,
           "The selected source/bytecode pair has no supported replacement compiler.");
    return;
  }
  if (!WorkIsCurrent(work) || !SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed, "The RTSR session changed before compilation started.");
    return;
  }

  rtsr::ShaderCompileInput input;
  input.Stage = work.Apply.Stage;
  input.D3DShaderVersion = work.Apply.D3DShaderVersion;
  input.EntrypointName = work.Apply.EntrypointName;
  input.OriginalEntrypointName = original->Shader.EntrypointName;
  input.Source = work.Apply.Source;
  input.TargetApi = work.Snapshot.Session.UsedApi;
  rtsr::ShaderCompileResult compiled =
      directX ? rtsr::CompileHlslToDxil(input, original->Shader.Bytecode, work.Apply.PlayerPath)
              : rtsr::CompileGlslToSpirv(input, original->Shader.Bytecode);
  if (!compiled) {
    Finish(work, OperationState::Failed, std::move(compiled.Diagnostic));
    return;
  }
  if (!SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed, "The RTSR session changed while compiling the shader.");
    return;
  }

  StageOverlays overlays;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto generation = m_PipelineGenerations.find(work.Apply.PipelineId);
    if (m_Stopping || work.ServiceGeneration != m_Generation ||
        generation == m_PipelineGenerations.end() ||
        generation->second != work.PipelineGeneration) {
      return;
    }
    const auto pipeline = m_Overlays.find(work.Apply.PipelineId);
    if (pipeline != m_Overlays.end()) {
      overlays = pipeline->second;
    }
  }

  rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO replacement;
  replacement.ShaderStage = work.Apply.Stage;
  replacement.Bytecode = std::move(compiled.Bytecode);
  replacement.BytecodeHash =
      rtsr::ComputeBytecodeHash(replacement.Bytecode.data(), replacement.Bytecode.size());
  replacement.EntrypointName = std::move(compiled.EntrypointName);
  replacement.BytecodeType = original->Shader.BytecodeType;
  replacement.D3DShaderVersion = original->Shader.D3DShaderVersion;
  overlays[work.Apply.Stage] = std::move(replacement);

  std::string diagnostic = std::move(compiled.Diagnostic);
  auto request = BuildUpdateRequest(work, overlays, diagnostic);
  if (!request.has_value()) {
    Finish(work, OperationState::Failed, std::move(diagnostic));
    return;
  }
  if (!WorkIsCurrent(work) || !SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed, "The RTSR session changed before the update was sent.");
    return;
  }

  const rtsr::ServerRequestResult result = m_Server.UpdatePipelines(*request);
  if (!result) {
    AppendDiagnostic(diagnostic, result.Diagnostic);
    Finish(work, OperationState::Failed, std::move(diagnostic));
    return;
  }
  if (!WorkIsCurrent(work) || !SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed,
           "The pipeline disappeared before the replacement result was committed.");
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto generation = m_PipelineGenerations.find(work.Apply.PipelineId);
    if (m_Stopping || work.ServiceGeneration != m_Generation ||
        generation == m_PipelineGenerations.end() ||
        generation->second != work.PipelineGeneration) {
      return;
    }
    m_Overlays[work.Apply.PipelineId] = std::move(overlays);
  }
  Finish(work, OperationState::Succeeded, std::move(diagnostic));
}

void RtsrReplacementService::Reset(Work work) {
  if (!WorkIsCurrent(work) || !SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed, "The selected pipeline is no longer available.");
    return;
  }

  const rtsr::RTSR_RESET_PIPELINES_REQUEST request{
      {{work.Snapshot.Pipeline.Id, work.Snapshot.Pipeline.Incarnation}}};
  const rtsr::ServerRequestResult result = m_Server.ResetPipelines(request);
  if (!result) {
    Finish(work, OperationState::Failed, result.Diagnostic);
    return;
  }
  if (!WorkIsCurrent(work) || !SnapshotIsCurrent(work.Snapshot)) {
    Finish(work, OperationState::Failed,
           "The pipeline disappeared before the reset result was committed.");
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto generation = m_PipelineGenerations.find(work.Snapshot.Pipeline.Id);
    if (m_Stopping || work.ServiceGeneration != m_Generation ||
        generation == m_PipelineGenerations.end() ||
        generation->second != work.PipelineGeneration) {
      return;
    }
    m_Overlays.erase(work.Snapshot.Pipeline.Id);
  }
  Finish(work, OperationState::Succeeded, {});
}

void RtsrReplacementService::Run() {
  for (;;) {
    Work work;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_Wake.wait(lock, [this] { return m_Stopping || !m_Queue.empty(); });
      if (m_Stopping) {
        return;
      }
      work = std::move(m_Queue.front());
      m_Queue.pop_front();
    }

    if (work.Kind == OperationKind::Apply) {
      ApplyShader(std::move(work));
    } else {
      Reset(std::move(work));
    }
  }
}

} // namespace gits::gui
