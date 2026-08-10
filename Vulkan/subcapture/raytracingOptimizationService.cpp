// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingOptimizationService.h"
#include "log.h"

namespace gits {
namespace vulkan {

void RaytracingOptimizationService::RecordBuild(
    uint64_t cbKey, uint64_t commandKey, uint64_t dstAsKey, uint64_t srcAsKey, bool isUpdate) {
  if (!dstAsKey) {
    return;
  }
  auto command = std::make_unique<RaytracingCommand>();
  command->CommandKey = commandKey;
  command->DstAsKey = dstAsKey;
  // A fresh build has no source. Only an update refits from a source AS.
  command->SrcAsKey = isUpdate ? srcAsKey : 0;
  command->IsUpdate = isUpdate;
  command->IsCopy = false;
  m_CommandsByCb[cbKey].push_back(std::move(command));
}

void RaytracingOptimizationService::RecordCopy(uint64_t cbKey,
                                               uint64_t commandKey,
                                               uint64_t dstAsKey,
                                               uint64_t srcAsKey,
                                               VkCopyAccelerationStructureModeKHR mode) {
  if (!dstAsKey) {
    return;
  }
  auto command = std::make_unique<RaytracingCommand>();
  command->CommandKey = commandKey;
  command->DstAsKey = dstAsKey;
  command->SrcAsKey = srcAsKey;
  command->IsUpdate = false;
  command->IsCopy = true;
  command->CopyMode = mode;
  m_CommandsByCb[cbKey].push_back(std::move(command));
}

void RaytracingOptimizationService::OnQueueSubmit(uint64_t cbKey) {
  auto it = m_CommandsByCb.find(cbKey);
  if (it == m_CommandsByCb.end()) {
    return;
  }
  for (auto& command : it->second) {
    StoreCommand(std::move(command));
  }
  m_CommandsByCb.erase(it);
}

void RaytracingOptimizationService::MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey) {
  auto it = m_CommandsByCb.find(secondaryKey);
  if (it == m_CommandsByCb.end() || it->second.empty()) {
    return;
  }
  auto& dst = m_CommandsByCb[primaryKey];
  for (auto& command : it->second) {
    dst.push_back(std::move(command));
  }
  m_CommandsByCb.erase(it);
}

void RaytracingOptimizationService::StoreCommand(std::unique_ptr<RaytracingCommand> commandPtr) {
  commandPtr->Id = ++m_CommandUniqueId;
  m_Commands.push_back(std::move(commandPtr));
  RaytracingCommand* command = m_Commands.back().get();

  RaytracingCommand* source = nullptr;
  if (command->SrcAsKey) {
    auto it = m_CommandByAs.find(command->SrcAsKey);
    if (it != m_CommandByAs.end()) {
      source = it->second;
    } else {
      // Malformed or truncated pre-range sequence. Treat as a root instead of aborting.
      LOG_WARNING << "Vulkan subcapture: acceleration structure op (command key="
                  << command->CommandKey << ") references source AS key=" << command->SrcAsKey
                  << " with no recorded producer; treating as a root";
    }
  }

  // Collapse a run of updates to {root build, last update}. Copies are not collapsed.
  if (command->IsUpdate && source && source->Source && source->IsUpdate) {
    source = source->Source;
    command->SrcAsKey = source->DstAsKey;
  }

  // Supersede the destination AS's producer. The old chain survives only while
  // reachable via another op's Source.
  m_CommandByAs[command->DstAsKey] = command;

  command->Source = source;
}

void RaytracingOptimizationService::Optimize(const std::unordered_set<uint64_t>& usedBlasKeys) {
  m_Optimized.clear();

  // Mark each used BLAS's final pre-cut writer for restoration.
  for (auto& [asKey, command] : m_CommandByAs) {
    if (usedBlasKeys.count(asKey)) {
      command->Restore = true;
    }
  }

  // Mark reduced chain for restoration.
  for (auto& commandPtr : m_Commands) {
    RaytracingCommand* command = commandPtr.get();
    if (command->Restore) {
      RaytracingCommand* source = command->Source;
      while (source) {
        source->Restore = true;
        source = source->Source;
      }
    }
  }

  // Emit retained ops in Id (execution) order. Preserving order is important.
  for (auto& commandPtr : m_Commands) {
    RaytracingCommand* command = commandPtr.get();
    if (!command->Restore) {
      continue;
    }
    OptimizedAsCommand out;
    out.CommandKey = command->CommandKey;
    out.SourceCommandKey = command->Source ? command->Source->CommandKey : 0;
    out.DstAsKey = command->DstAsKey;
    out.SrcAsKey = command->SrcAsKey;
    out.IsCopy = command->IsCopy;
    out.CopyMode = command->CopyMode;
    m_Optimized.push_back(out);
  }
}

} // namespace vulkan
} // namespace gits
