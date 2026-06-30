// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerService.h"
#include "analyzerResults.h"
#include "stateTrackingService.h"
#include "objectState.h"
#include "configurator.h"
#include "log.h"

#include "yaml-cpp/yaml.h"

#include <fstream>

namespace gits {
namespace vulkan {

AnalyzerService::AnalyzerService(StateTrackingService& stateTracking,
                                 SubcaptureRange& subcaptureRange)
    : m_StateTracking(stateTracking), m_SubcaptureRange(subcaptureRange) {
  m_Optimize = Configurator::Get().common.features.subcapture.optimize;
}

AnalyzerService::~AnalyzerService() {
  // Safety net: if the stream ended while still inside the range (e.g. the
  // application exited mid-subcapture) make sure the collected information is
  // still written, mirroring the DirectX AnalyzerService destructor.
  try {
    if (!m_Dumped && !m_ObjectsForRestore.empty()) {
      DumpAnalysisFile();
    }
  } catch (...) {
    // Destructors must not throw.
  }
}

void AnalyzerService::NotifyObject(uint64_t objectKey) {
  if (m_Optimize && objectKey && m_SubcaptureRange.InRange()) {
    m_ObjectsForRestore.insert(objectKey);
  }
}

void AnalyzerService::NotifyObjects(const std::vector<uint64_t>& objectKeys) {
  if (!m_Optimize || !m_SubcaptureRange.InRange()) {
    return;
  }
  for (uint64_t key : objectKeys) {
    if (key) {
      m_ObjectsForRestore.insert(key);
    }
  }
}

void AnalyzerService::AddClosure(uint64_t key, std::set<uint64_t>& outKeys) {
  if (!key) {
    return;
  }
  if (!outKeys.insert(key).second) {
    return; // already visited
  }

  ObjectState* state = m_StateTracking.GetState(key);
  if (!state) {
    return;
  }

  // Generic relationships shared by every object type.
  AddClosure(state->ParentKey, outKeys);
  for (uint64_t dep : state->DependencyKeys) {
    AddClosure(dep, outKeys);
  }

  // Type-specific links that are stored outside DependencyKeys.  These must be
  // part of the restore set so that the gated post-restore passes (memory bind,
  // image-layout transitions, content upload, descriptor allocation, etc.) have
  // every object they reference available.
  switch (state->CreationCommandId) {
  case CommandId::ID_VKCREATEBUFFER:
    AddClosure(static_cast<BufferState*>(state)->BoundMemoryKey, outKeys);
    break;
  case CommandId::ID_VKCREATEIMAGE:
    AddClosure(static_cast<ImageState*>(state)->BoundMemoryKey, outKeys);
    break;
  case CommandId::ID_VKALLOCATEDESCRIPTORSETS: {
    auto* ds = static_cast<DescriptorSetState*>(state);
    AddClosure(ds->PoolKey, outKeys);
    AddClosure(ds->LayoutKey, outKeys);
    break;
  }
  case CommandId::ID_VKALLOCATECOMMANDBUFFERS:
    AddClosure(static_cast<CommandBufferState*>(state)->PoolKey, outKeys);
    break;
  case CommandId::ID_VKCREATESWAPCHAINKHR:
    for (uint64_t imgKey : static_cast<SwapchainState*>(state)->ImageKeys) {
      AddClosure(imgKey, outKeys);
    }
    break;
  default:
    break;
  }
}

void AnalyzerService::DumpAnalysisFile() {
  if (m_Dumped) {
    return;
  }
  m_Dumped = true;

  std::set<uint64_t> closure;
  for (uint64_t key : m_ObjectsForRestore) {
    AddClosure(key, closure);
  }

  // Emit YAML in a deterministic order so the completion marker ("Complete") is
  // the very last thing written.  If the analysis run is interrupted (crash /
  // kill) the file is left truncated without that marker, which the loader
  // (AnalyzerResults) detects and treats as an incomplete/corrupt analysis.
  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  emitter << YAML::Key << "Objects" << YAML::Value << YAML::BeginSeq;
  for (uint64_t key : closure) {
    if (key) {
      emitter << key;
    }
  }
  emitter << YAML::EndSeq;
  // Completion marker -- emitted last on purpose (see comment above).
  emitter << YAML::Key << "Complete" << YAML::Value << true;
  emitter << YAML::EndMap;

  const std::string fileName = AnalyzerResults::GetAnalysisFileName();
  std::ofstream out(fileName);
  if (!out) {
    LOG_ERROR << "Vulkan2 subcapture: failed to open analysis file '" << fileName
              << "' for writing";
    return;
  }
  out << emitter.c_str() << "\n";

  LOG_INFO << "Vulkan2 subcapture: analysis written (" << m_ObjectsForRestore.size()
           << " used objects, " << closure.size() << " objects in restore closure)";
}

} // namespace vulkan
} // namespace gits
