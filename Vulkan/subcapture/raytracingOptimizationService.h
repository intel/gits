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
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

// Reduces the pre-range chain of acceleration-structure operations (build / update /
// copy) per BLAS to the minimal set needed to reconstruct its final pre-cut contents,
// and prunes chains no in-range consumer references. Micromaps get the same treatment
// through a second, independent graph - their ops form chains for the same reason
// (vkCmdCopyMicromapEXT links one micromap's chain to another).
//
// Reduction rules:
//  - A run of updates collapses to {root build, last update}.
//  - Copies (CLONE/COMPACT) are never collapsed - each links one object's chain to another.
//  - A build/copy into an object supersedes its chain, which is then dropped unless still
//    reachable as another retained op's source.
//
// Ops are staged per command buffer and flushed at submit, so the graph follows GPU
// execution order across multiple command buffers and resubmits.
class RaytracingOptimizationService {
public:
  // One retained operation, in replay (chronological Id) order.
  struct OptimizedAsCommand {
    uint64_t CommandKey{};       // stable command key (matches the recording pass)
    uint64_t SourceCommandKey{}; // reduced-chain source op (0 for a root build)
    uint64_t DstAsKey{};         // destination AS (kept alive in the closure)
    uint64_t SrcAsKey{};         // source AS after collapse (0 if none)
    bool IsCopy{};
    VkCopyAccelerationStructureModeKHR CopyMode{}; // valid only when IsCopy
  };

  // The micromap counterpart. Same fields, and no update mode exists for micromaps so
  // SrcMicromapKey is non-zero only for a copy.
  struct OptimizedMicromapCommand {
    uint64_t CommandKey{};
    uint64_t SourceCommandKey{};
    uint64_t DstMicromapKey{};
    uint64_t SrcMicromapKey{};
    bool IsCopy{};
    VkCopyMicromapModeEXT CopyMode{}; // valid only when IsCopy
  };

  // A retained acceleration structure build read a micromap, and a later op then
  // overwrote that micromap. The restore materializes only a micromap's final content,
  // so the build would be replayed against the wrong data - see OptimizeMicromaps.
  struct MicromapOverwriteConflict {
    uint64_t MicromapKey{};
    uint64_t ConsumedCommandKey{}; // op that produced the content the AS build read
    uint64_t AsBuildCommandKey{};  // the retained AS build that read it
    uint64_t OverwritingCommandKey{};
  };

  // One call per destination AS. srcAsKey is the update-mode source AS (0 for a fresh
  // build). Caller must skip TLAS builds.
  void RecordBuild(
      uint64_t cbKey, uint64_t commandKey, uint64_t dstAsKey, uint64_t srcAsKey, bool isUpdate);

  // CLONE/COMPACT copies only.
  void RecordCopy(uint64_t cbKey,
                  uint64_t commandKey,
                  uint64_t dstAsKey,
                  uint64_t srcAsKey,
                  VkCopyAccelerationStructureModeKHR mode);

  // One call per destination micromap. Micromaps have a single build mode, so every
  // build is a chain root.
  void RecordMicromapBuild(uint64_t cbKey, uint64_t commandKey, uint64_t dstMicromapKey);

  // CLONE/COMPACT copies only - the serialize variants are separate commands.
  void RecordMicromapCopy(uint64_t cbKey,
                          uint64_t commandKey,
                          uint64_t dstMicromapKey,
                          uint64_t srcMicromapKey,
                          VkCopyMicromapModeEXT mode);

  // Micromaps an acceleration structure build names in a pNext chain, so commandKey is an
  // acceleration structure command here. Feeds the side table that pulls them into the restore
  // closure, plus an OmmReadByRtasBuild node in the micromap graph.
  void RecordAsBuildMicromapReads(uint64_t cbKey,
                                  uint64_t commandKey,
                                  const std::vector<uint64_t>& micromapKeys);

  // Empty when the command names no micromap.
  const std::vector<uint64_t>& GetAsBuildMicromaps(uint64_t commandKey) const;

  // Flush the command buffer's staged ops through both chain graphs in record order.
  void OnQueueSubmit(uint64_t cbKey);

  // Fold a secondary command buffer's staged ops into the primary at
  // vkCmdExecuteCommands, so they flush when the primary is submitted.
  void MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey);

  // Populates GetOptimizedCommands(). Call once at range end.
  void Optimize(const std::unordered_set<uint64_t>& usedBlasKeys);

  const std::vector<OptimizedAsCommand>& GetOptimizedCommands() const {
    return m_Optimized;
  }

  // Populates GetOptimizedMicromapCommands() and GetMicromapOverwriteConflicts(). Must run
  // after Optimize(), which decides retainedAsCommandKeys.
  void OptimizeMicromaps(const std::unordered_set<uint64_t>& usedMicromapKeys,
                         const std::unordered_set<uint64_t>& retainedAsCommandKeys);

  const std::vector<OptimizedMicromapCommand>& GetOptimizedMicromapCommands() const {
    return m_OptimizedMicromaps;
  }

  const std::vector<MicromapOverwriteConflict>& GetMicromapOverwriteConflicts() const {
    return m_MicromapOverwriteConflicts;
  }

private:
  enum class NodeKind {
    Build,  // writes DstKey from scratch
    Update, // writes DstKey, refitting from SrcKey
    Copy,   // writes DstKey from SrcKey, CopyMode valid
    // Reads DstKey rather than writing it: an acceleration structure build consumed this
    // micromap. Records only which content that build saw, so it neither supersedes the
    // destination's producer nor appears in the results, and CommandKey names an AS-graph command.
    OmmReadByRtasBuild,
  };

  // One node of a chain graph. Object-kind agnostic: CopyMode is untyped so the same graph
  // serves acceleration structures and micromaps.
  struct ChainNode {
    uint64_t Id{};
    // The command this node stands for - see NodeKind::OmmReadByRtasBuild for the one kind
    // where that is a command of the other graph.
    uint64_t CommandKey{};
    // The object written, or the object read on an OmmReadByRtasBuild.
    uint64_t DstKey{};
    uint64_t SrcKey{};
    NodeKind Kind{NodeKind::Build};
    uint32_t CopyMode{};
    // The op that produced this node's input, or on an OmmReadByRtasBuild the op whose output
    // it saw.
    ChainNode* Source{};
    bool Restore{};
  };

  // The reduction itself, instantiated once per object kind. objectKind names that kind in
  // log messages, the only place the graph is not kind-agnostic.
  class ChainGraph {
  public:
    explicit ChainGraph(const char* objectKind) : m_ObjectKind(objectKind) {}

    void Record(uint64_t cbKey, std::unique_ptr<ChainNode> node);
    void Flush(uint64_t cbKey);
    void MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey);
    // Marks the final producer of every used key, plus its reduced chain, for restoration.
    void Optimize(const std::unordered_set<uint64_t>& usedKeys);
    // Retained nodes in Id (execution) order, reads excluded. Valid after Optimize().
    std::vector<const ChainNode*> Retained() const;
    // Every staged-and-flushed OmmReadByRtasBuild node, in Id order.
    std::vector<const ChainNode*> OmmReadsByRtasBuild() const;
    // The op that currently produces dstKey, or nullptr.
    const ChainNode* CurrentProducer(uint64_t dstKey) const;

  private:
    void Store(std::unique_ptr<ChainNode> node);

    const char* m_ObjectKind{};
    uint64_t m_UniqueId{};
    // Ops staged per recording command buffer, flushed at submit.
    std::unordered_map<uint64_t, std::vector<std::unique_ptr<ChainNode>>> m_NodesByCb;
    // Current producer node per destination key.
    std::unordered_map<uint64_t, ChainNode*> m_NodeByDst;
    // All flushed nodes, owned, in Id (execution) order.
    std::vector<std::unique_ptr<ChainNode>> m_Nodes;
  };

  ChainGraph m_AsGraph{"acceleration structure"};
  ChainGraph m_MicromapGraph{"micromap"};

  std::vector<OptimizedAsCommand> m_Optimized;
  std::vector<OptimizedMicromapCommand> m_OptimizedMicromaps;
  std::vector<MicromapOverwriteConflict> m_MicromapOverwriteConflicts;
  std::unordered_map<uint64_t, std::vector<uint64_t>> m_MicromapsByAsCommand;
};

} // namespace vulkan
} // namespace gits
