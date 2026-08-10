// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerService.h"
#include "analyzerResults.h"
#include "analyzerRaytracingService.h"
#include "raytracingOptimizationService.h"
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
  m_Optimize = Configurator::Get().common.player.subcapture.optimize;
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

void AnalyzerService::AddObjectForRestore(uint64_t objectKey) {
  if (m_Optimize && objectKey && m_SubcaptureRange.InRange()) {
    m_ObjectsForRestore.insert(objectKey);
  }
}

void AnalyzerService::AddObjectsForRestore(const std::vector<uint64_t>& objectKeys) {
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
    // Resources bound into the set are referenced only through it, not via
    // ParentKey/DependencyKeys. A TLAS reaches the closure only here, and it in turn
    // pulls in the BLASes it references.
    std::vector<uint64_t> boundKeys;
    m_StateTracking.GetDescriptorSetUpdateService().CollectBoundKeys(ds->Key, boundKeys);
    for (uint64_t boundKey : boundKeys) {
      AddClosure(boundKey, outKeys);
    }
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
  case CommandId::ID_VKCREATEACCELERATIONSTRUCTUREKHR: {
    // Only the TLAS->BLAS edge. The backing buffer is already in DependencyKeys.
    auto* as = static_cast<AccelerationStructureState*>(state);
    if (as->Type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR && m_RaytracingService) {
      for (uint64_t blasKey : m_RaytracingService->GetReferencedBlases(key)) {
        AddClosure(blasKey, outKeys);
      }
    }
    break;
  }
  default:
    break;
  }
}

void AnalyzerService::AddDeviceAddressBufferClosure(std::set<uint64_t>& outKeys) {
  if (!m_Optimize) {
    return;
  }

  // The closure walk follows Vulkan handle references only. A buffer whose address the
  // application baked into other memory (an SBT record, a push constant, another
  // buffer's contents) is reachable by no handle, so it would be trimmed while the
  // address naming it is restored verbatim - the replayed shader then dereferences
  // unmapped memory. The usage bit alone is not enough of a signal: acceleration
  // structure scratch carries it too, hence the DeviceAddress check.
  size_t retained = 0;
  for (const auto& [key, state] : m_StateTracking.GetStates()) {
    if (!key || state->Destroyed || state->CreationCommandId != CommandId::ID_VKCREATEBUFFER) {
      continue;
    }
    auto* buffer = static_cast<BufferState*>(state.get());
    if (!(buffer->UsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) ||
        buffer->DeviceAddress == 0) {
      continue;
    }
    if (outKeys.count(key)) {
      continue; // already reachable by handle
    }
    AddClosure(key, outKeys); // also pulls in the bound memory and parent device
    ++retained;
  }

  if (retained) {
    LOG_INFO << "Vulkan subcapture: retained " << retained
             << " buffer(s) reachable only by device address";
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
  AddDeviceAddressBufferClosure(closure);

  // Reduce each used BLAS's pre-range op chain to the minimal restore set. The "used"
  // set is every BLAS in the closure, which covers both "referenced by an in-range
  // TLAS" and "used directly by an in-range command".
  std::vector<RaytracingOptimizationService::OptimizedAsCommand> blasChain;
  if (m_OptimizationService) {
    std::unordered_set<uint64_t> usedBlasKeys;
    for (uint64_t key : closure) {
      // Destroyed acceleration structures stay in the state map for the chain replay,
      // but nothing live can reference one, so they are not "used".
      auto* as = m_StateTracking.GetState<AccelerationStructureState>(key);
      if (as && !as->Destroyed && as->Type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR) {
        usedBlasKeys.insert(key);
      }
    }
    m_OptimizationService->Optimize(usedBlasKeys);
    blasChain = m_OptimizationService->GetOptimizedCommands();

    // Seed the closure with every AS a retained op touches, so source structures no
    // consumer references directly still survive to be rebuilt during the chain replay.
    for (const auto& op : blasChain) {
      AddClosure(op.DstAsKey, closure);
      AddClosure(op.SrcAsKey, closure);
    }
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
  // Retained BLAS chain ops, in replay (execution) order. Emitted as flow maps (one
  // line per op) because a chain can hold tens of thousands of entries.
  emitter << YAML::Key << "BlasChain" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : blasChain) {
    emitter << YAML::Flow << YAML::BeginMap;
    emitter << YAML::Key << "Cmd" << YAML::Value << op.CommandKey;
    emitter << YAML::Key << "DstAs" << YAML::Value << op.DstAsKey;
    emitter << YAML::Key << "SrcCmd" << YAML::Value << op.SourceCommandKey;
    emitter << YAML::Key << "SrcAs" << YAML::Value << op.SrcAsKey;
    emitter << YAML::Key << "IsCopy" << YAML::Value << (op.IsCopy ? 1 : 0);
    if (op.IsCopy) {
      emitter << YAML::Key << "CopyMode" << YAML::Value << static_cast<int>(op.CopyMode);
    }
    emitter << YAML::EndMap;
  }
  emitter << YAML::EndSeq;
  // Completion marker -- emitted last on purpose (see comment above).
  emitter << YAML::Key << "Complete" << YAML::Value << true;
  emitter << YAML::EndMap;

  const std::string fileName = AnalyzerResults::GetAnalysisFileName();
  std::ofstream out(fileName);
  if (!out) {
    LOG_ERROR << "Vulkan subcapture: failed to open analysis file '" << fileName << "' for writing";
    return;
  }
  out << emitter.c_str() << "\n";

  LOG_INFO << "Vulkan subcapture: analysis written (" << m_ObjectsForRestore.size()
           << " used objects, " << closure.size() << " objects in restore closure)";
}

} // namespace vulkan
} // namespace gits
