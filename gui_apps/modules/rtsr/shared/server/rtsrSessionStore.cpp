// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/server/rtsrSessionStore.h"

#include <utility>

namespace gits::rtsr {

void SessionStore::ClearLocked() {
  m_Pipelines.clear();
  m_PipelineRevisions.clear();
  m_Shaders.clear();
  m_NextPipelineRevision = 0;
}

void SessionStore::ReleaseStagesLocked(const std::vector<PipelineStage>& stages) {
  for (const auto& stage : stages) {
    auto it = m_Shaders.find(stage.BytecodeHash);
    if (it == m_Shaders.end()) {
      continue;
    }
    if (it->second.ReferenceCount > 1) {
      --it->second.ReferenceCount;
    } else {
      m_Shaders.erase(it);
    }
  }
}

void SessionStore::OpenSession(SessionInfo info) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  ClearLocked();
  ++m_Generation;
  m_Info = std::move(info);
  m_SessionOpen = true;
}

void SessionStore::CloseSession() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SessionOpen = false;
}

void SessionStore::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  ClearLocked();
  ++m_Generation;
  m_Info = {};
  m_SessionOpen = false;
}

bool SessionStore::IsSessionOpen() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_SessionOpen;
}

SessionInfo SessionStore::Info() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Info;
}

size_t SessionStore::PipelineCount() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Pipelines.size();
}

size_t SessionStore::ShaderCount() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Shaders.size();
}

std::vector<PipelineStage> SessionStore::AddPipeline(RTSR_PIPELINE_CREATED_MESSAGE message) {
  std::vector<PipelineStage> newShaders;

  PipelineRecord record;
  record.Id = message.Id;
  record.Type = message.Type;
  record.State = std::move(message.State);
  record.StateHash = message.StateHash;
  record.Incarnation = message.Incarnation;
  record.ReplacementEligible = message.ReplacementEligible;
  record.ReplacementDiagnostic = std::move(message.ReplacementDiagnostic);
  record.Stages.reserve(message.ShaderStages.size());

  std::lock_guard<std::mutex> lock(m_Mutex);

  // A repeated id means the client reused it, so the previous incarnation is
  // gone and its shader references go with it.
  auto existing = m_Pipelines.find(message.Id);
  if (existing != m_Pipelines.end()) {
    ReleaseStagesLocked(existing->second.Stages);
    // The name is the only thing worth carrying over: it is sent separately and
    // may have arrived before the pipeline was re-created.
    record.Name = std::move(existing->second.Name);
    m_Pipelines.erase(existing);
  }

  for (auto& stage : message.ShaderStages) {
    const PipelineStage binding{stage.ShaderStage, stage.BytecodeHash, stage.EntrypointName};
    record.Stages.push_back(binding);

    auto [it, inserted] = m_Shaders.try_emplace(stage.BytecodeHash);
    if (inserted) {
      it->second.BytecodeType = stage.BytecodeType;
      it->second.EntrypointName = std::move(stage.EntrypointName);
      it->second.D3DShaderVersion = stage.D3DShaderVersion;
      it->second.Bytecode = std::move(stage.Bytecode);
      newShaders.push_back(binding);
    }
    ++it->second.ReferenceCount;
  }

  m_PipelineRevisions[record.Id] = ++m_NextPipelineRevision;
  m_Pipelines.emplace(record.Id, std::move(record));
  return newShaders;
}

bool SessionStore::SetPipelineEligibility(const RTSR_PIPELINE_ELIGIBILITY_MESSAGE& message) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto pipeline = m_Pipelines.find(message.PipelineId);
  if (!m_SessionOpen || m_Info.UsedApi < RTSR_API_VULKAN_1_0 ||
      m_Info.UsedApi > RTSR_API_VULKAN_1_3 || pipeline == m_Pipelines.end() ||
      pipeline->second.Incarnation != message.Incarnation) {
    return false;
  }
  pipeline->second.ReplacementEligible = message.ReplacementEligible;
  pipeline->second.ReplacementDiagnostic = message.Diagnostic;
  m_PipelineRevisions[message.PipelineId] = ++m_NextPipelineRevision;
  return true;
}

bool SessionStore::SetPipelineName(RTSR_ID id, std::string name) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Pipelines.find(id);
  if (it == m_Pipelines.end()) {
    return false;
  }
  it->second.Name = std::move(name);
  return true;
}

bool SessionStore::RemovePipeline(RTSR_ID id) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Pipelines.find(id);
  if (it == m_Pipelines.end()) {
    return false;
  }
  ReleaseStagesLocked(it->second.Stages);
  m_Pipelines.erase(it);
  m_PipelineRevisions[id] = ++m_NextPipelineRevision;
  return true;
}

std::optional<PipelineRecord> SessionStore::Pipeline(RTSR_ID id) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Pipelines.find(id);
  if (it == m_Pipelines.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<ShaderSummary> SessionStore::PipelineShaders(RTSR_ID id) const {
  std::vector<ShaderSummary> summaries;

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto pipeline = m_Pipelines.find(id);
  if (pipeline == m_Pipelines.end()) {
    return summaries;
  }

  summaries.reserve(pipeline->second.Stages.size());
  for (const auto& stage : pipeline->second.Stages) {
    ShaderSummary summary;
    summary.BytecodeHash = stage.BytecodeHash;
    summary.Stage = stage.Stage;

    auto shader = m_Shaders.find(stage.BytecodeHash);
    if (shader != m_Shaders.end()) {
      summary.BytecodeType = shader->second.BytecodeType;
      summary.EntrypointName = stage.EntrypointName;
      summary.D3DShaderVersion = shader->second.D3DShaderVersion;
      summary.BytecodeSize = shader->second.Bytecode.size();
    }
    summaries.push_back(std::move(summary));
  }
  return summaries;
}

std::optional<SessionStore::ShaderBlob> SessionStore::Shader(
    RTSR_HASH hash, std::string_view entrypointName) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_Shaders.find(hash);
  if (it == m_Shaders.end()) {
    return std::nullopt;
  }
  const auto& shader = it->second;
  return ShaderBlob{shader.BytecodeType, std::string(entrypointName), shader.D3DShaderVersion,
                    shader.Bytecode};
}

std::optional<SessionStore::ReplacementSnapshot> SessionStore::SnapshotForReplacement(
    RTSR_ID id) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_SessionOpen) {
    return std::nullopt;
  }

  const auto pipeline = m_Pipelines.find(id);
  if (pipeline == m_Pipelines.end()) {
    return std::nullopt;
  }

  ReplacementSnapshot snapshot;
  snapshot.Generation = m_Generation;
  snapshot.PipelineRevision = m_PipelineRevisions.at(id);
  snapshot.Session = m_Info;
  snapshot.Pipeline = pipeline->second;
  snapshot.Shaders.reserve(pipeline->second.Stages.size());

  for (const PipelineStage& binding : pipeline->second.Stages) {
    const auto shader = m_Shaders.find(binding.BytecodeHash);
    if (shader == m_Shaders.end() || shader->second.Bytecode.empty()) {
      return std::nullopt;
    }
    const auto& record = shader->second;
    snapshot.Shaders.push_back(
        {binding.Stage,
         binding.BytecodeHash,
         {record.BytecodeType, binding.EntrypointName, record.D3DShaderVersion, record.Bytecode}});
  }
  return snapshot;
}

bool SessionStore::IsCurrent(const ReplacementSnapshot& snapshot) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_SessionOpen || m_Generation != snapshot.Generation) {
    return false;
  }
  const auto pipeline = m_Pipelines.find(snapshot.Pipeline.Id);
  const auto revision = m_PipelineRevisions.find(snapshot.Pipeline.Id);
  if (pipeline == m_Pipelines.end() || revision == m_PipelineRevisions.end() ||
      revision->second != snapshot.PipelineRevision ||
      pipeline->second.Type != snapshot.Pipeline.Type ||
      pipeline->second.StateHash != snapshot.Pipeline.StateHash ||
      pipeline->second.Incarnation != snapshot.Pipeline.Incarnation ||
      pipeline->second.State != snapshot.Pipeline.State ||
      pipeline->second.Stages.size() != snapshot.Shaders.size()) {
    return false;
  }
  for (size_t i = 0; i < snapshot.Shaders.size(); ++i) {
    if (pipeline->second.Stages[i].Stage != snapshot.Shaders[i].Stage ||
        pipeline->second.Stages[i].BytecodeHash != snapshot.Shaders[i].BytecodeHash ||
        pipeline->second.Stages[i].EntrypointName != snapshot.Shaders[i].Shader.EntrypointName) {
      return false;
    }
  }
  return true;
}

} // namespace gits::rtsr
