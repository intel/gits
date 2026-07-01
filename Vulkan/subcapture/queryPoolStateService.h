// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Tracks the per-query reset / used state of every VkQueryPool so the
// subcapture state-restore pass can re-create "available" queries.
//
// A query written before the subcapture cut (e.g. a timestamp resolved in a
// frame that precedes the recording range) leaves the recording range with a
// query it never wrote itself.  The recording range then calls
// vkGetQueryPoolResults on it; on a freshly created (uninitialized) pool that
// read returns VK_ERROR_DEVICE_LOST.  To avoid this we record which queries
// were reset / written before the cut and, during restore, reset them and
// issue a fake query (vkCmdWriteTimestamp, or vkCmdBeginQuery + vkCmdEndQuery)
// so the result is available.
//
// Queries take effect on the GPU at submit time, not at vkCmd* record time, so
// the per-command-buffer effects are buffered in CommandBufferState
// (reset/UsedQueriesAfterSubmit) and applied to the pool when the CB is
// submitted.  Mirrors the legacy vulkanStateTracking / RestoreQueryPool logic.
//
// Extracted from SubcaptureLayer / StateTrackingService to keep all query-pool
// state bookkeeping in one place.  The restore-time command emission lives in
// StateTrackingService::RestoreQueryPools (where the transient command-buffer
// infrastructure already exists).
class QueryPoolStateService {
public:
  explicit QueryPoolStateService(StateTrackingService& sts);

  // Called from Post(vkCreateQueryPool) to size the per-query bitmaps.
  void OnCreateQueryPool(uint64_t poolKey, uint32_t queryType, uint32_t queryCount);

  // Called from Post(vkCmdResetQueryPool): buffers a reset of [firstQuery,
  // firstQuery + queryCount) into the recording command buffer.
  void OnCmdResetQueryPool(uint64_t cbKey,
                           uint64_t poolKey,
                           uint32_t firstQuery,
                           uint32_t queryCount);

  // Called from the query-writing Post handlers (vkCmdBeginQuery,
  // vkCmdEndQuery, vkCmdWriteTimestamp[2][KHR]): buffers "query used" into the
  // recording command buffer.
  void OnCmdUseQuery(uint64_t cbKey, uint64_t poolKey, uint32_t query);

  // Called from Post(vkCmdExecuteCommands) for each secondary CB: folds the
  // secondary's buffered query effects into the primary so they are applied
  // when the primary is submitted.
  void MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey);

  // Called per submitted command buffer (from the queue-submit path): applies
  // the CB's buffered reset / used effects to the referenced QueryPoolStates.
  void ApplyCommandBuffer(uint64_t cbKey);

private:
  // Records the queue family of the command buffer's command pool onto the
  // query pool, so the restore pass replays query commands on a capable queue.
  void RecordQueueFamily(uint64_t cbKey, uint64_t poolKey);

  StateTrackingService& m_StateTracking;
};

} // namespace vulkan
} // namespace gits
