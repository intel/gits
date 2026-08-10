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
// and prunes chains no in-range consumer references.
//
// Reduction rules:
//  - A run of updates collapses to {root build, last update}.
//  - Copies (CLONE/COMPACT) are never collapsed - each links one AS's chain to another.
//  - A build/copy into an AS supersedes its chain, which is then dropped unless still
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

  // Flush the command buffer's staged ops through the chain graph in record order.
  void OnQueueSubmit(uint64_t cbKey);

  // Fold a secondary command buffer's staged ops into the primary at
  // vkCmdExecuteCommands, so they flush when the primary is submitted.
  void MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey);

  // Populates GetOptimizedCommands(). Call once at range end.
  void Optimize(const std::unordered_set<uint64_t>& usedBlasKeys);

  const std::vector<OptimizedAsCommand>& GetOptimizedCommands() const {
    return m_Optimized;
  }

private:
  struct RaytracingCommand {
    uint64_t Id{};
    uint64_t CommandKey{};
    uint64_t DstAsKey{};
    uint64_t SrcAsKey{};
    bool IsUpdate{};
    bool IsCopy{};
    VkCopyAccelerationStructureModeKHR CopyMode{};
    RaytracingCommand* Source{};
    bool Restore{};
  };

  void StoreCommand(std::unique_ptr<RaytracingCommand> command);

  uint64_t m_CommandUniqueId{};
  // Ops staged per recording command buffer, flushed at submit.
  std::unordered_map<uint64_t, std::vector<std::unique_ptr<RaytracingCommand>>> m_CommandsByCb;
  // Current producer node per destination AS key.
  std::unordered_map<uint64_t, RaytracingCommand*> m_CommandByAs;
  // All flushed nodes, owned, in Id (execution) order.
  std::vector<std::unique_ptr<RaytracingCommand>> m_Commands;

  std::vector<OptimizedAsCommand> m_Optimized;
};

} // namespace vulkan
} // namespace gits
