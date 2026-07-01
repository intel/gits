// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "queryPoolStateService.h"
#include "stateTrackingService.h"
#include "objectState.h"

namespace gits {
namespace vulkan {

QueryPoolStateService::QueryPoolStateService(StateTrackingService& sts) : m_StateTracking(sts) {}

// Remember which queue family the application used to touch this query pool, so
// the restore pass replays its query commands on a query-capable queue.
void QueryPoolStateService::RecordQueueFamily(uint64_t cbKey, uint64_t poolKey) {
  auto* qp = m_StateTracking.GetState<QueryPoolState>(poolKey);
  if (!qp) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  auto* cmdPool = m_StateTracking.GetState<CommandPoolState>(cb->PoolKey);
  if (cmdPool && cmdPool->QueueFamilyIndex != UINT32_MAX) {
    qp->RestoreQueueFamily = cmdPool->QueueFamilyIndex;
  }
}

void QueryPoolStateService::OnCreateQueryPool(uint64_t poolKey,
                                              uint32_t queryType,
                                              uint32_t queryCount) {
  auto* qp = m_StateTracking.GetState<QueryPoolState>(poolKey);
  if (!qp) {
    return;
  }
  qp->QueryType = queryType;
  qp->QueryCount = queryCount;
  qp->ResetQueries.assign(queryCount, false);
  qp->UsedQueries.assign(queryCount, false);
}

void QueryPoolStateService::OnCmdResetQueryPool(uint64_t cbKey,
                                                uint64_t poolKey,
                                                uint32_t firstQuery,
                                                uint32_t queryCount) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb || !poolKey) {
    return;
  }
  auto& reset = cb->ResetQueriesAfterSubmit[poolKey];
  for (uint32_t i = firstQuery; i < firstQuery + queryCount; ++i) {
    reset.insert(i);
  }
  RecordQueueFamily(cbKey, poolKey);
}

void QueryPoolStateService::OnCmdUseQuery(uint64_t cbKey, uint64_t poolKey, uint32_t query) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb || !poolKey) {
    return;
  }
  cb->UsedQueriesAfterSubmit[poolKey].insert(query);
  RecordQueueFamily(cbKey, poolKey);
}

void QueryPoolStateService::MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey) {
  auto* prim = m_StateTracking.GetState<CommandBufferState>(primaryKey);
  auto* sec = m_StateTracking.GetState<CommandBufferState>(secondaryKey);
  if (!prim || !sec) {
    return;
  }
  for (const auto& [poolKey, indices] : sec->ResetQueriesAfterSubmit) {
    prim->ResetQueriesAfterSubmit[poolKey].insert(indices.begin(), indices.end());
  }
  for (const auto& [poolKey, indices] : sec->UsedQueriesAfterSubmit) {
    prim->UsedQueriesAfterSubmit[poolKey].insert(indices.begin(), indices.end());
  }
}

void QueryPoolStateService::ApplyCommandBuffer(uint64_t cbKey) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  // Resets first, then uses: a query that is both reset and written by the same
  // submit ends up reset == true and used == true (it is available).  Matches
  // the legacy ordering in vkQueueSubmit_SD.
  for (const auto& [poolKey, indices] : cb->ResetQueriesAfterSubmit) {
    auto* qp = m_StateTracking.GetState<QueryPoolState>(poolKey);
    if (!qp) {
      continue;
    }
    for (uint32_t i : indices) {
      if (i < qp->ResetQueries.size()) {
        qp->ResetQueries[i] = true;
        qp->UsedQueries[i] = false;
      }
    }
  }
  for (const auto& [poolKey, indices] : cb->UsedQueriesAfterSubmit) {
    auto* qp = m_StateTracking.GetState<QueryPoolState>(poolKey);
    if (!qp) {
      continue;
    }
    for (uint32_t i : indices) {
      if (i < qp->UsedQueries.size()) {
        qp->UsedQueries[i] = true;
      }
    }
  }
}

} // namespace vulkan
} // namespace gits
