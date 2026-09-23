// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingOptimizationService.h"
#include "log.h"

#include <algorithm>

namespace gits {
namespace vulkan {

// ---------------------------------------------------------------------------
// ChainGraph
// ---------------------------------------------------------------------------

void RaytracingOptimizationService::ChainGraph::Record(uint64_t cbKey,
                                                       std::unique_ptr<ChainNode> node) {
  m_NodesByCb[cbKey].push_back(std::move(node));
}

void RaytracingOptimizationService::ChainGraph::Flush(uint64_t cbKey) {
  auto it = m_NodesByCb.find(cbKey);
  if (it == m_NodesByCb.end()) {
    return;
  }
  for (auto& node : it->second) {
    Store(std::move(node));
  }
  m_NodesByCb.erase(it);
}

void RaytracingOptimizationService::ChainGraph::MergeSecondary(uint64_t primaryKey,
                                                               uint64_t secondaryKey) {
  auto it = m_NodesByCb.find(secondaryKey);
  if (it == m_NodesByCb.end() || it->second.empty()) {
    return;
  }
  auto& dst = m_NodesByCb[primaryKey];
  for (auto& node : it->second) {
    dst.push_back(std::move(node));
  }
  m_NodesByCb.erase(it);
}

void RaytracingOptimizationService::ChainGraph::Store(std::unique_ptr<ChainNode> nodePtr) {
  nodePtr->Id = ++m_UniqueId;
  m_Nodes.push_back(std::move(nodePtr));
  ChainNode* node = m_Nodes.back().get();

  if (node->Kind == NodeKind::OmmReadByRtasBuild) {
    // Whatever produced the destination at this point in execution order, or nullptr when
    // nothing pre-range did. A read never supersedes that producer, and a missing one is
    // ordinary here - the object may be written in range, or never.
    auto it = m_NodeByDst.find(node->DstKey);
    node->Source = it == m_NodeByDst.end() ? nullptr : it->second;
    return;
  }

  ChainNode* source = nullptr;
  if (node->SrcKey) {
    auto it = m_NodeByDst.find(node->SrcKey);
    if (it != m_NodeByDst.end()) {
      source = it->second;
    } else {
      // Malformed or truncated pre-range sequence. Treat as a root instead of aborting.
      LOG_WARNING << "Vulkan subcapture: " << m_ObjectKind
                  << " op (command key=" << node->CommandKey
                  << ") references source key=" << node->SrcKey
                  << " with no recorded producer; treating as a root";
    }
  }

  // Collapse a run of updates to {root build, last update}. Copies are not collapsed.
  if (node->Kind == NodeKind::Update && source && source->Source &&
      source->Kind == NodeKind::Update) {
    source = source->Source;
    node->SrcKey = source->DstKey;
  }

  // Supersede the destination's producer. The old chain survives only while
  // reachable via another op's Source.
  m_NodeByDst[node->DstKey] = node;

  node->Source = source;
}

void RaytracingOptimizationService::ChainGraph::Optimize(
    const std::unordered_set<uint64_t>& usedKeys) {
  // Mark each used object's final pre-cut writer for restoration.
  for (auto& [dstKey, node] : m_NodeByDst) {
    if (usedKeys.count(dstKey)) {
      node->Restore = true;
    }
  }

  // Mark reduced chain for restoration.
  for (auto& nodePtr : m_Nodes) {
    ChainNode* node = nodePtr.get();
    if (node->Restore) {
      ChainNode* source = node->Source;
      while (source) {
        source->Restore = true;
        source = source->Source;
      }
    }
  }
}

std::vector<const RaytracingOptimizationService::ChainNode*> RaytracingOptimizationService::
    ChainGraph::Retained() const {
  std::vector<const ChainNode*> retained;
  // Id (execution) order. Preserving order is important.
  for (const auto& nodePtr : m_Nodes) {
    const ChainNode* node = nodePtr.get();
    // A read writes nothing, so it is never replayed. Its Restore stays false anyway (nothing
    // points at it), but the reduction should not depend on that.
    if (node->Restore && node->Kind != NodeKind::OmmReadByRtasBuild) {
      retained.push_back(node);
    }
  }
  return retained;
}

std::vector<const RaytracingOptimizationService::ChainNode*> RaytracingOptimizationService::
    ChainGraph::OmmReadsByRtasBuild() const {
  std::vector<const ChainNode*> reads;
  for (const auto& nodePtr : m_Nodes) {
    if (nodePtr->Kind == NodeKind::OmmReadByRtasBuild) {
      reads.push_back(nodePtr.get());
    }
  }
  return reads;
}

const RaytracingOptimizationService::ChainNode* RaytracingOptimizationService::ChainGraph::
    CurrentProducer(uint64_t dstKey) const {
  auto it = m_NodeByDst.find(dstKey);
  return it == m_NodeByDst.end() ? nullptr : it->second;
}

// ---------------------------------------------------------------------------
// Acceleration structures
// ---------------------------------------------------------------------------

void RaytracingOptimizationService::RecordBuild(
    uint64_t cbKey, uint64_t commandKey, uint64_t dstAsKey, uint64_t srcAsKey, bool isUpdate) {
  if (!dstAsKey) {
    return;
  }
  auto node = std::make_unique<ChainNode>();
  node->CommandKey = commandKey;
  node->DstKey = dstAsKey;
  // A fresh build has no source. Only an update refits from a source AS.
  node->SrcKey = isUpdate ? srcAsKey : 0;
  node->Kind = isUpdate ? NodeKind::Update : NodeKind::Build;
  m_AsGraph.Record(cbKey, std::move(node));
}

void RaytracingOptimizationService::RecordCopy(uint64_t cbKey,
                                               uint64_t commandKey,
                                               uint64_t dstAsKey,
                                               uint64_t srcAsKey,
                                               VkCopyAccelerationStructureModeKHR mode) {
  if (!dstAsKey) {
    return;
  }
  auto node = std::make_unique<ChainNode>();
  node->CommandKey = commandKey;
  node->DstKey = dstAsKey;
  node->SrcKey = srcAsKey;
  node->Kind = NodeKind::Copy;
  node->CopyMode = static_cast<uint32_t>(mode);
  m_AsGraph.Record(cbKey, std::move(node));
}

void RaytracingOptimizationService::Optimize(const std::unordered_set<uint64_t>& usedBlasKeys) {
  m_Optimized.clear();
  m_AsGraph.Optimize(usedBlasKeys);

  for (const ChainNode* node : m_AsGraph.Retained()) {
    OptimizedAsCommand out;
    out.CommandKey = node->CommandKey;
    out.SourceCommandKey = node->Source ? node->Source->CommandKey : 0;
    out.DstAsKey = node->DstKey;
    out.SrcAsKey = node->SrcKey;
    // The analysis file keeps a bool here - its format should not follow an internal refactor.
    out.IsCopy = node->Kind == NodeKind::Copy;
    out.CopyMode = static_cast<VkCopyAccelerationStructureModeKHR>(node->CopyMode);
    m_Optimized.push_back(out);
  }
}

// ---------------------------------------------------------------------------
// Micromaps
// ---------------------------------------------------------------------------

void RaytracingOptimizationService::RecordMicromapBuild(uint64_t cbKey,
                                                        uint64_t commandKey,
                                                        uint64_t dstMicromapKey) {
  if (!dstMicromapKey) {
    return;
  }
  auto node = std::make_unique<ChainNode>();
  node->CommandKey = commandKey;
  node->DstKey = dstMicromapKey;
  node->Kind = NodeKind::Build;
  m_MicromapGraph.Record(cbKey, std::move(node));
}

void RaytracingOptimizationService::RecordMicromapCopy(uint64_t cbKey,
                                                       uint64_t commandKey,
                                                       uint64_t dstMicromapKey,
                                                       uint64_t srcMicromapKey,
                                                       VkCopyMicromapModeEXT mode) {
  if (!dstMicromapKey) {
    return;
  }
  auto node = std::make_unique<ChainNode>();
  node->CommandKey = commandKey;
  node->DstKey = dstMicromapKey;
  node->SrcKey = srcMicromapKey;
  node->Kind = NodeKind::Copy;
  node->CopyMode = static_cast<uint32_t>(mode);
  m_MicromapGraph.Record(cbKey, std::move(node));
}

void RaytracingOptimizationService::RecordAsBuildMicromapReads(
    uint64_t cbKey, uint64_t commandKey, const std::vector<uint64_t>& micromapKeys) {
  if (micromapKeys.empty()) {
    return;
  }
  auto& stored = m_MicromapsByAsCommand[commandKey];
  // A command buffer can be resubmitted, so the same command key arrives more than once with
  // the same keys. Overwrite rather than append.
  stored = micromapKeys;

  // Staged like the ops themselves, so a second sighting of this command stages a second set
  // of reads. Harmless - they resolve to the same producer, and the conflict report dedupes.
  for (uint64_t micromapKey : micromapKeys) {
    if (!micromapKey) {
      continue;
    }
    auto read = std::make_unique<ChainNode>();
    read->CommandKey = commandKey;
    read->DstKey = micromapKey;
    read->Kind = NodeKind::OmmReadByRtasBuild;
    m_MicromapGraph.Record(cbKey, std::move(read));
  }
}

const std::vector<uint64_t>& RaytracingOptimizationService::GetAsBuildMicromaps(
    uint64_t commandKey) const {
  static const std::vector<uint64_t> empty;
  auto it = m_MicromapsByAsCommand.find(commandKey);
  return it == m_MicromapsByAsCommand.end() ? empty : it->second;
}

void RaytracingOptimizationService::OptimizeMicromaps(
    const std::unordered_set<uint64_t>& usedMicromapKeys,
    const std::unordered_set<uint64_t>& retainedAsCommandKeys) {
  m_OptimizedMicromaps.clear();
  m_MicromapOverwriteConflicts.clear();
  m_MicromapGraph.Optimize(usedMicromapKeys);

  for (const ChainNode* node : m_MicromapGraph.Retained()) {
    OptimizedMicromapCommand out;
    out.CommandKey = node->CommandKey;
    out.SourceCommandKey = node->Source ? node->Source->CommandKey : 0;
    out.DstMicromapKey = node->DstKey;
    out.SrcMicromapKey = node->SrcKey;
    out.IsCopy = node->Kind == NodeKind::Copy;
    out.CopyMode = static_cast<VkCopyMicromapModeEXT>(node->CopyMode);
    m_OptimizedMicromaps.push_back(out);
  }

  // A micromap's restore reproduces one content - whatever its final pre-range op wrote. A
  // retained acceleration structure build that read an *earlier* content would therefore be
  // replayed against the wrong data, silently. Report those so the caller can refuse.
  for (const ChainNode* read : m_MicromapGraph.OmmReadsByRtasBuild()) {
    if (!retainedAsCommandKeys.count(read->CommandKey)) {
      continue; // the build is not replayed, so what it read does not matter
    }
    const ChainNode* consumed = read->Source;
    if (!consumed) {
      continue; // nothing pre-range produced it - the unproduced-micromap path covers this
    }
    const ChainNode* latest = m_MicromapGraph.CurrentProducer(read->DstKey);
    if (!latest || latest == consumed) {
      continue;
    }
    MicromapOverwriteConflict conflict;
    conflict.MicromapKey = read->DstKey;
    conflict.ConsumedCommandKey = consumed->CommandKey;
    conflict.AsBuildCommandKey = read->CommandKey;
    conflict.OverwritingCommandKey = latest->CommandKey;
    const bool known = std::any_of(m_MicromapOverwriteConflicts.begin(),
                                   m_MicromapOverwriteConflicts.end(), [&conflict](const auto& c) {
                                     return c.MicromapKey == conflict.MicromapKey &&
                                            c.AsBuildCommandKey == conflict.AsBuildCommandKey;
                                   });
    if (!known) {
      m_MicromapOverwriteConflicts.push_back(conflict);
    }
  }
}

// ---------------------------------------------------------------------------
// Shared plumbing
// ---------------------------------------------------------------------------

void RaytracingOptimizationService::OnQueueSubmit(uint64_t cbKey) {
  m_AsGraph.Flush(cbKey);
  m_MicromapGraph.Flush(cbKey);
}

void RaytracingOptimizationService::MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey) {
  m_AsGraph.MergeSecondary(primaryKey, secondaryKey);
  m_MicromapGraph.MergeSecondary(primaryKey, secondaryKey);
}

} // namespace vulkan
} // namespace gits
