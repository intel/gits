// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "uniformLocationMapService.h"

#include "log.h"

#include <algorithm>

namespace gits::opengl2 {

UniformLocationMapService& UniformLocationMapService::Get() {
  static UniformLocationMapService instance;
  return instance;
}

void UniformLocationMapService::Register(GITSKey programKey,
                                         GLint capturedLocation,
                                         GLint replayLocation) {
  if (programKey == 0 || capturedLocation < 0) {
    return;
  }
  const Key key{programKey, capturedLocation};
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Locations[key] = replayLocation;
  m_WarnedMissing.erase(key);
}

GLint UniformLocationMapService::Resolve(HGLRC context, GLint capturedLocation) {
  return ResolveForProgram(GetUniformProgram(context), capturedLocation);
}

GLint UniformLocationMapService::ResolveForProgram(GITSKey programKey, GLint capturedLocation) {
  if (capturedLocation < 0) {
    return capturedLocation; // -1 (or another negative sentinel) round-trips as itself - no
                             // real uniform location is ever negative.
  }
  const Key key{programKey, capturedLocation};
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Locations.find(key);
  if (it != m_Locations.end()) {
    return it->second;
  }
  if (m_WarnedMissing.insert(key).second) {
    LOG_WARNING << "UniformLocationMapService: no registered replay-time location for program "
                   "key "
                << programKey << ", captured location " << capturedLocation
                << " - replaying the captured value unchanged, which may set the wrong uniform "
                   "(or none) if this replay's driver assigned different locations. This "
                   "happens for a location value the application computed itself instead of "
                   "obtaining directly from glGetUniformLocation (e.g. array-element offset "
                   "arithmetic) - see uniformLocationMapService.h's own scope decision.";
  }
  return capturedLocation;
}

std::unordered_map<GLint, std::vector<GLint>> UniformLocationMapService::CapturedLocations(
    GITSKey programKey) const {
  std::unordered_map<GLint, std::vector<GLint>> locations;
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (const auto& [key, replayLocation] : m_Locations) {
    if (key.ProgramKey == programKey && replayLocation >= 0) {
      locations[replayLocation].push_back(key.CapturedLocation);
    }
  }
  for (auto& [replayLocation, captured] : locations) {
    std::sort(captured.begin(), captured.end());
  }
  return locations;
}

void UniformLocationMapService::SetCurrentProgram(HGLRC context, GITSKey programKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_CurrentProgramByContext.find(context);
  const GITSKey outgoing = (it != m_CurrentProgramByContext.end()) ? it->second : 0;
  m_CurrentProgramByContext[context] = programKey;

  // Complete a deferred PurgeProgram() now if the program `context` is switching away from was
  // pending deletion and has just dropped to zero remaining current contexts - see
  // PurgeProgram()'s own comment. Nothing to do if `context` is re-binding the same program it
  // already had (outgoing == programKey - not a real transition) or was not current anywhere
  // to begin with.
  if (outgoing != programKey) {
    ReleaseProgramLocked(outgoing);
  }
}

GITSKey UniformLocationMapService::GetCurrentProgram(HGLRC context) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_CurrentProgramByContext.find(context);
  return it != m_CurrentProgramByContext.end() ? it->second : 0;
}

void UniformLocationMapService::BindProgramPipeline(HGLRC context, GITSKey pipelineKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PipelineByContext[context] = pipelineKey;
}

void UniformLocationMapService::SetActiveProgram(GITSKey pipelineKey, GITSKey programKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  GITSKey& active = m_ActiveProgramByPipeline[pipelineKey];
  const GITSKey outgoing = active;
  active = programKey;
  if (outgoing != programKey) {
    ReleaseProgramLocked(outgoing);
  }
}

void UniformLocationMapService::DeleteProgramPipeline(GITSKey pipelineKey) {
  if (pipelineKey == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const GITSKey active = ActiveProgramLocked(pipelineKey);
  m_ActiveProgramByPipeline.erase(pipelineKey);
  std::erase_if(m_PipelineByContext,
                [pipelineKey](const auto& entry) { return entry.second == pipelineKey; });
  ReleaseProgramLocked(active);
}

GITSKey UniformLocationMapService::GetUniformProgram(HGLRC context) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (const auto program = m_CurrentProgramByContext.find(context);
      program != m_CurrentProgramByContext.end() && program->second != 0) {
    return program->second;
  }
  const auto pipeline = m_PipelineByContext.find(context);
  return pipeline != m_PipelineByContext.end() ? ActiveProgramLocked(pipeline->second) : 0;
}

GITSKey UniformLocationMapService::ActiveProgramLocked(GITSKey pipelineKey) const {
  const auto active = m_ActiveProgramByPipeline.find(pipelineKey);
  return active != m_ActiveProgramByPipeline.end() ? active->second : 0;
}

void UniformLocationMapService::ReleaseProgramLocked(GITSKey programKey) {
  if (programKey != 0 && m_PendingDeletion.count(programKey) != 0 &&
      !IsCurrentInAnyContextLocked(programKey)) {
    ErasePurgeProgramLocked(programKey);
    m_PendingDeletion.erase(programKey);
  }
}

void UniformLocationMapService::PurgeProgram(GITSKey programKey) {
  if (programKey == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (IsCurrentInAnyContextLocked(programKey)) {
    // Deferred: still legally usable (further glUniform*() calls included) in at least one
    // context - see this method's own header comment. SetCurrentProgram() completes the purge
    // once that stops being true.
    m_PendingDeletion.insert(programKey);
    return;
  }
  ErasePurgeProgramLocked(programKey);
}

bool UniformLocationMapService::IsCurrentInAnyContextLocked(GITSKey programKey) const {
  for (const auto& entry : m_CurrentProgramByContext) {
    if (entry.second == programKey) {
      return true;
    }
  }
  // The active program of a pipeline stays usable by glUniform*() calls once the pipeline is bound.
  for (const auto& entry : m_ActiveProgramByPipeline) {
    if (entry.second == programKey) {
      return true;
    }
  }
  return false;
}

void UniformLocationMapService::ErasePurgeProgramLocked(GITSKey programKey) {
  for (auto it = m_Locations.begin(); it != m_Locations.end();) {
    it = (it->first.ProgramKey == programKey) ? m_Locations.erase(it) : std::next(it);
  }
  for (auto it = m_WarnedMissing.begin(); it != m_WarnedMissing.end();) {
    it = (it->ProgramKey == programKey) ? m_WarnedMissing.erase(it) : std::next(it);
  }
}

} // namespace gits::opengl2
