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
#include "subcaptureFatal.h"
#include "log.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>

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

void AnalyzerService::NoteInRangeAsWrite(uint64_t asKey) {
  if (m_Optimize && asKey && m_SubcaptureRange.InRange()) {
    m_AsWrittenInRange.insert(asKey);
  }
}

void AnalyzerService::NoteInRangeAsRead(uint64_t asKey) {
  if (m_Optimize && asKey && m_SubcaptureRange.InRange()) {
    m_AsReadInRange.insert(asKey);
  }
}

void AnalyzerService::NoteInRangeMicromapWrite(uint64_t micromapKey) {
  if (m_Optimize && micromapKey && m_SubcaptureRange.InRange()) {
    m_MicromapWrittenInRange.insert(micromapKey);
  }
}

void AnalyzerService::NoteInRangeMicromapRead(uint64_t micromapKey) {
  if (m_Optimize && micromapKey && m_SubcaptureRange.InRange()) {
    m_MicromapReadInRange.insert(micromapKey);
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
  case CommandId::ID_VKCREATEBUFFER: {
    auto* buf = static_cast<BufferState*>(state);
    AddClosure(buf->BoundMemoryKey, outKeys);
    // A sparse buffer has no single bound memory - its pages come from any
    // number of allocations, and all of them have to survive the trim or the
    // buffer replays with holes.
    for (const auto& [offset, range] : buf->SparseRanges) {
      AddClosure(range.MemoryKey, outKeys);
    }
    break;
  }
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
  std::vector<RaytracingOptimizationService::OptimizedMicromapCommand> micromapChain;
  if (m_OptimizationService) {
    std::unordered_set<uint64_t> usedBlasKeys;
    for (uint64_t key : closure) {
      // Only a structure destroyed before the range is unusable by an in-range command.
      auto* as = m_StateTracking.GetState<AccelerationStructureState>(key);
      if (as && !as->DestroyedBeforeRange &&
          as->Type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR) {
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
      if (!op.IsCopy) {
        for (uint64_t mmKey : m_OptimizationService->GetAsBuildMicromaps(op.CommandKey)) {
          AddClosure(mmKey, closure);
        }
      }
    }

    // Reduce the micromap chains the same way, seeded from the closure the acceleration
    // structure pass just grew - a micromap matters if an in-range command or a retained
    // pre-range build can reach it. Must run after Optimize, which decides retention.
    std::unordered_set<uint64_t> retainedAsCommandKeys;
    for (const auto& op : blasChain) {
      retainedAsCommandKeys.insert(op.CommandKey);
    }
    // No DestroyedBeforeRange filter, unlike the acceleration structures above: a micromap
    // destroyed before the range is still needed when a retained pre-range build names it,
    // because that build is replayed during state restore. Being in the closure is the test.
    std::unordered_set<uint64_t> usedMicromapKeys;
    for (uint64_t key : closure) {
      if (m_StateTracking.GetState<MicromapState>(key)) {
        usedMicromapKeys.insert(key);
      }
    }
    m_OptimizationService->OptimizeMicromaps(usedMicromapKeys, retainedAsCommandKeys);
    micromapChain = m_OptimizationService->GetOptimizedMicromapCommands();
    // A compaction source no consumer names directly still has to survive to be rebuilt
    // during the chain replay, exactly as for acceleration structures.
    for (const auto& op : micromapChain) {
      AddClosure(op.DstMicromapKey, closure);
      AddClosure(op.SrcMicromapKey, closure);
    }

    // Same failure mode as the unproduced structures below: a micromap the range reads but
    // never writes replays uninitialized, and an acceleration structure built over it is
    // malformed. A micromap the range builds itself is legitimately left empty by the restore.
    if (Configurator::Get().common.player.subcapture.vulkan.captureASBuildInputs) {
      std::unordered_set<uint64_t> micromapChainDstKeys;
      for (const auto& op : micromapChain) {
        micromapChainDstKeys.insert(op.DstMicromapKey);
      }
      std::vector<uint64_t> unproducedMicromaps;
      for (uint64_t key : m_MicromapReadInRange) {
        if (!m_MicromapWrittenInRange.count(key) && !micromapChainDstKeys.count(key) &&
            closure.count(key)) {
          unproducedMicromaps.push_back(key);
        }
      }
      if (!unproducedMicromaps.empty()) {
        std::ostringstream keys;
        for (size_t i = 0; i < unproducedMicromaps.size() && i < 8; ++i) {
          keys << (i ? ", " : "") << unproducedMicromaps[i];
        }
        if (unproducedMicromaps.size() > 8) {
          keys << ", ... (" << unproducedMicromaps.size() << " total)";
        }
        LOG_WARNING << "Vulkan subcapture: " << unproducedMicromaps.size()
                    << " micromap(s) are read inside the range but never written there, and no "
                       "retained operation produces them - the acceleration structures built "
                       "over them will be malformed: keys "
                    << keys.str();
      }
    }

    // The restore reproduces one content per micromap - whatever its last pre-range op wrote.
    // A retained build that read an earlier content would be replayed over the wrong opacity
    // data, which no later pass can detect, so refuse instead.
    for (const auto& conflict : m_OptimizationService->GetMicromapOverwriteConflicts()) {
      FatalSubcaptureError(
          "micromap key=" + std::to_string(conflict.MicromapKey) +
          " is overwritten by command key=" + std::to_string(conflict.OverwritingCommandKey) +
          " after acceleration structure build command key=" +
          std::to_string(conflict.AsBuildCommandKey) + " consumed the content written by " +
          "command key=" + std::to_string(conflict.ConsumedCommandKey) +
          ". That build is retained by the restore, which can only reproduce the micromap's "
          "final content, so subcapturing this stream would rebuild it over the wrong opacity "
          "data");
    }

    // Anything the range reads but never writes has to come out of the restore. A structure
    // no retained op produces replays uninitialized, which the driver reads as a malformed
    // acceleration structure - typically a device loss a frame or two later, far from the
    // cause. Most structures the restore leaves empty are fine (nothing reads them, or an
    // in-range build fills them first), so only this set is worth reporting.
    std::unordered_set<uint64_t> chainDstKeys;
    for (const auto& op : blasChain) {
      chainDstKeys.insert(op.DstAsKey);
    }
    std::vector<uint64_t> unproduced;
    // Only meaningful when the recording pass will actually restore from this chain. With
    // captureASBuildInputs off every structure comes from a serialized blob instead, so an
    // empty chain says nothing (AnalyzerResults::UseAsChainRestore).
    if (Configurator::Get().common.player.subcapture.vulkan.captureASBuildInputs) {
      for (uint64_t key : m_AsReadInRange) {
        if (m_AsWrittenInRange.count(key) || chainDstKeys.count(key) || !closure.count(key)) {
          continue;
        }
        // Top-level structures come from the separate TLAS rebuild path, not the chain.
        auto* as = m_StateTracking.GetState<AccelerationStructureState>(key);
        if (as && as->Type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR) {
          unproduced.push_back(key);
        }
      }
    }
    if (!unproduced.empty()) {
      std::ostringstream keys;
      for (size_t i = 0; i < unproduced.size() && i < 8; ++i) {
        keys << (i ? ", " : "") << unproduced[i];
      }
      if (unproduced.size() > 8) {
        keys << ", ... (" << unproduced.size() << " total)";
      }
      LOG_WARNING << "Vulkan subcapture: " << unproduced.size()
                  << " bottom-level acceleration structure(s) are read inside the range but "
                     "never written there, and no retained chain operation produces them - "
                     "they will replay uninitialized and the stream is likely to lose the "
                     "device: keys "
                  << keys.str();
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
  // Retained micromap chain ops, same shape and ordering guarantees as BlasChain above.
  // Always emitted, even when empty: its absence is what tells the recording pass the file
  // predates micromap copy support (AnalyzerResults::HasMicromapChain).
  emitter << YAML::Key << "MicromapChain" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : micromapChain) {
    emitter << YAML::Flow << YAML::BeginMap;
    emitter << YAML::Key << "Cmd" << YAML::Value << op.CommandKey;
    emitter << YAML::Key << "DstMicromap" << YAML::Value << op.DstMicromapKey;
    emitter << YAML::Key << "SrcCmd" << YAML::Value << op.SourceCommandKey;
    emitter << YAML::Key << "SrcMicromap" << YAML::Value << op.SrcMicromapKey;
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
