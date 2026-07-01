// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandBufferLifecycleService.h"
#include "stateTrackingService.h"
#include "objectState.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"
#include "commandIdsAuto.h"

namespace gits {
namespace vulkan {

CommandBufferLifecycleService::CommandBufferLifecycleService(StateTrackingService& sts)
    : m_StateTracking(sts) {}

// static
void CommandBufferLifecycleService::ClearState(CommandBufferState& state) {
  state.IsRecording = false;
  state.IsExecutable = false;
  state.BeginFlags = 0;
  state.DependencyKeys.clear();
  state.BeginCommandBuffer.clear();
  state.EndCommandBuffer.clear();
  state.RecordedCommands.clear();
  state.RecordedCommandIds.clear();
  state.EventStatesAfterSubmit.clear();
  state.ResetQueriesAfterSubmit.clear();
  state.UsedQueriesAfterSubmit.clear();
  state.ImageLayoutAfterSubmit.clear();
}

void CommandBufferLifecycleService::OnAllocate(vkAllocateCommandBuffersCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pAllocateInfo.Value) {
    return;
  }

  const uint64_t poolKey =
      command.m_pAllocateInfo.HandleKeys.empty() ? 0 : command.m_pAllocateInfo.HandleKeys[0];

  // Build a modified VkCommandBufferAllocateInfo with commandBufferCount=1 so
  // that each stored CB state emits a single-CB vkAllocateCommandBuffers during
  // state restore.  Storing the original batch command in every CB's state
  // would re-emit the N-CB allocation once per CB (N times total), producing
  // N handles and corrupting the handle-map.
  VkCommandBufferAllocateInfo singleAllocInfo = *command.m_pAllocateInfo.Value;
  singleAllocInfo.commandBufferCount = 1;

  // The owning pool state holds the per-pool CB index used by OnResetPool /
  // OnDestroyPool; grab it once.  std::map is node-based so this pointer stays
  // valid across the StoreState insertions below.
  auto* poolState = m_StateTracking.GetState<CommandPoolState>(poolKey);
  if (poolState) {
    poolState->AllocatedCommandBufferKeys.reserve(poolState->AllocatedCommandBufferKeys.size() +
                                                  command.m_pCommandBuffers.Size);
  }

  for (uint32_t i = 0; i < command.m_pCommandBuffers.Size; ++i) {
    const uint64_t cbKey = command.m_pCommandBuffers.Keys[i];
    auto state = std::make_unique<CommandBufferState>();
    state->Key = cbKey;
    state->ParentKey = command.m_device.Key;
    state->PoolKey = poolKey;
    // Index this CB under its pool so reset/destroy can walk only this pool's
    // buffers.  Only real (stored) keys are indexed; StoreState skips key 0.
    // Keys are unique per allocated CB, so this never inserts a duplicate.
    if (poolState && cbKey) {
      poolState->AllocatedCommandBufferKeys.insert(cbKey);
    }

    // Synthetic single-CB allocation command for this CB only.
    vkAllocateCommandBuffersCommand singleCmd;
    singleCmd.m_device = command.m_device;
    singleCmd.m_pAllocateInfo.Value = &singleAllocInfo;
    singleCmd.m_pAllocateInfo.HandleKeys = command.m_pAllocateInfo.HandleKeys;
    // Value must be non-null: GetSize/Encode for HandleArrayOutputArgument
    // short-circuit to a null-pointer flag when Value is null, causing the
    // second player to receive a null pCommandBuffers and crash in the driver.
    // Point to the i-th element of the original output array (Encode uses
    // Keys, not Value, so the actual pointer content doesn't matter).
    singleCmd.m_pCommandBuffers.Value = command.m_pCommandBuffers.Value + i;
    singleCmd.m_pCommandBuffers.Size = 1;
    singleCmd.m_pCommandBuffers.Keys = {command.m_pCommandBuffers.Keys[i]};
    singleCmd.m_Return.Value = VK_SUCCESS;

    state->CreationCommandId = singleCmd.GetId();
    uint32_t size = GetSize(singleCmd);
    state->CreationCommandBuffer.resize(size);
    Encode(singleCmd, state->CreationCommandBuffer.data());
    m_StateTracking.StoreState(std::move(state));
  }
}

void CommandBufferLifecycleService::OnFree(const std::vector<uint64_t>& cbKeys) {
  for (uint64_t key : cbKeys) {
    // Keep the owning pool's CB index consistent before dropping the state.
    // unordered_set::erase is O(1) average, so titles that free command buffers
    // one at a time stay cheap.
    if (auto* cbState = m_StateTracking.GetState<CommandBufferState>(key)) {
      if (auto* poolState = m_StateTracking.GetState<CommandPoolState>(cbState->PoolKey)) {
        poolState->AllocatedCommandBufferKeys.erase(key);
      }
    }
    m_StateTracking.RemoveState(key);
  }
}

void CommandBufferLifecycleService::OnBegin(vkBeginCommandBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto* state = m_StateTracking.GetState<CommandBufferState>(command.m_commandBuffer.Key);
  if (!state) {
    return;
  }
  // vkBeginCommandBuffer implicitly resets the CB - clear any previous recording.
  ClearState(*state);
  state->IsRecording = true;
  state->BeginFlags = (command.m_pBeginInfo.Value ? command.m_pBeginInfo.Value->flags : 0);

  uint32_t size = GetSize(command);
  state->BeginCommandBuffer.resize(size);
  Encode(command, state->BeginCommandBuffer.data());
}

void CommandBufferLifecycleService::OnEnd(vkEndCommandBufferCommand& command) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(command.m_commandBuffer.Key);
  if (!state) {
    return;
  }
  // Per spec: if vkEndCommandBuffer fails the CB enters the invalid state and
  // is not executable.  Keep our tracking consistent with that so RestoreState
  // does not try to submit an unrecordable CB.
  if (command.m_Return.Value != VK_SUCCESS) {
    state->IsRecording = false;
    state->IsExecutable = false;
    state->EndCommandBuffer.clear();
    return;
  }
  // The CB transitions from recording to executable.  Keep BeginCommandBuffer
  // and RecordedCommands: an executable CB can be submitted multiple times
  // (unless it has ONE_TIME_SUBMIT_BIT) and must be re-closed during restore.
  state->IsRecording = false;
  state->IsExecutable = true;

  uint32_t size = GetSize(command);
  state->EndCommandBuffer.resize(size);
  Encode(command, state->EndCommandBuffer.data());
}

void CommandBufferLifecycleService::OnReset(uint64_t cbKey) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (state) {
    ClearState(*state);
  }
}

void CommandBufferLifecycleService::OnResetPool(uint64_t poolKey) {
  // vkResetCommandPool resets ALL command buffers in the pool to initial state,
  // regardless of their individual reset flag.  Walk only this pool's CB index
  // (maintained by OnAllocate / OnFree) instead of scanning every tracked
  // object -- O(buffers in this pool) rather than O(all objects).
  auto* poolState = m_StateTracking.GetState<CommandPoolState>(poolKey);
  if (!poolState) {
    // Unknown / already-destroyed pool: nothing to reset.
    return;
  }
  for (uint64_t cbKey : poolState->AllocatedCommandBufferKeys) {
    if (auto* cbState = m_StateTracking.GetState<CommandBufferState>(cbKey)) {
      ClearState(*cbState);
    }
  }
}

void CommandBufferLifecycleService::OnDestroyPool(uint64_t poolKey) {
  // vkDestroyCommandPool implicitly frees all command buffers allocated from
  // the pool.  Remove them from state tracking so RestoreOne does not try to
  // emit vkAllocateCommandBuffers referencing the now-gone pool key.  Walk only
  // this pool's CB index instead of scanning every tracked object.  RemoveState
  // erases from m_States but does not touch this vector, so iterate it directly
  // and clear the index afterwards (the pool state itself is removed by the
  // caller right after this returns).
  auto* poolState = m_StateTracking.GetState<CommandPoolState>(poolKey);
  if (!poolState) {
    return;
  }
  for (uint64_t cbKey : poolState->AllocatedCommandBufferKeys) {
    m_StateTracking.RemoveState(cbKey);
  }
  poolState->AllocatedCommandBufferKeys.clear();
}

void CommandBufferLifecycleService::TrackHandleDependency(uint64_t cbKey, uint64_t handleKey) {
  if (!handleKey) {
    return;
  }
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (state && state->IsRecording) {
    state->DependencyKeys.push_back(handleKey);
  }
}

void CommandBufferLifecycleService::TrackHandleDependencies(
    uint64_t cbKey, const std::vector<uint64_t>& handleKeys) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!state || !state->IsRecording) {
    return;
  }
  for (uint64_t key : handleKeys) {
    if (key) {
      state->DependencyKeys.push_back(key);
    }
  }
}

} // namespace vulkan
} // namespace gits
