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
  state.isRecording = false;
  state.isExecutable = false;
  state.beginFlags = 0;
  state.dependencyKeys.clear();
  state.beginCommandBuffer.clear();
  state.endCommandBuffer.clear();
  state.recordedCommands.clear();
  state.recordedCommandIds.clear();
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

  for (uint32_t i = 0; i < command.m_pCommandBuffers.Size; ++i) {
    auto state = std::make_unique<CommandBufferState>();
    state->key = command.m_pCommandBuffers.Keys[i];
    state->parentKey = command.m_device.Key;
    state->poolKey = poolKey;

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

    state->creationCommandId = singleCmd.GetId();
    uint32_t size = GetSize(singleCmd);
    state->creationCommandBuffer.resize(size);
    Encode(singleCmd, state->creationCommandBuffer.data());
    m_StateTracking.StoreState(std::move(state));
  }
}

void CommandBufferLifecycleService::OnFree(const std::vector<uint64_t>& cbKeys) {
  for (uint64_t key : cbKeys) {
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
  state->isRecording = true;
  state->beginFlags = (command.m_pBeginInfo.Value ? command.m_pBeginInfo.Value->flags : 0);

  uint32_t size = GetSize(command);
  state->beginCommandBuffer.resize(size);
  Encode(command, state->beginCommandBuffer.data());
}

void CommandBufferLifecycleService::OnEnd(vkEndCommandBufferCommand& command) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(command.m_commandBuffer.Key);
  if (!state) {
    return;
  }
  // The CB transitions from recording to executable.  Keep beginCommandBuffer
  // and recordedCommands: an executable CB can be submitted multiple times
  // (unless it has ONE_TIME_SUBMIT_BIT) and must be re-closed during restore.
  state->isRecording = false;
  state->isExecutable = true;

  uint32_t size = GetSize(command);
  state->endCommandBuffer.resize(size);
  Encode(command, state->endCommandBuffer.data());
}

void CommandBufferLifecycleService::OnReset(uint64_t cbKey) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (state) {
    ClearState(*state);
  }
}

void CommandBufferLifecycleService::OnResetPool(uint64_t poolKey) {
  // vkResetCommandPool resets ALL command buffers in the pool to initial state,
  // regardless of their individual reset flag.
  for (auto& [_, statePtr] : m_StateTracking.GetStates()) {
    ObjectState* objState = statePtr.get();
    if (!objState || objState->creationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
      continue;
    }
    auto* cbState = static_cast<CommandBufferState*>(objState);
    if (cbState->poolKey == poolKey) {
      ClearState(*cbState);
    }
  }
}

void CommandBufferLifecycleService::OnDestroyPool(uint64_t poolKey) {
  // vkDestroyCommandPool implicitly frees all command buffers allocated from
  // the pool.  Remove them from state tracking so RestoreOne does not try to
  // emit vkAllocateCommandBuffers referencing the now-gone pool key.
  std::vector<uint64_t> toRemove;
  for (auto& [key, statePtr] : m_StateTracking.GetStates()) {
    ObjectState* objState = statePtr.get();
    if (!objState || objState->creationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
      continue;
    }
    auto* cbState = static_cast<CommandBufferState*>(objState);
    if (cbState->poolKey == poolKey) {
      toRemove.push_back(key);
    }
  }
  for (uint64_t key : toRemove) {
    m_StateTracking.RemoveState(key);
  }
}

void CommandBufferLifecycleService::TrackHandleDependency(uint64_t cbKey, uint64_t handleKey) {
  if (!handleKey) {
    return;
  }
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (state && state->isRecording) {
    state->dependencyKeys.push_back(handleKey);
  }
}

void CommandBufferLifecycleService::TrackHandleDependencies(
    uint64_t cbKey, const std::vector<uint64_t>& handleKeys) {
  auto* state = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!state || !state->isRecording) {
    return;
  }
  for (uint64_t key : handleKeys) {
    if (key) {
      state->dependencyKeys.push_back(key);
    }
  }
}

} // namespace vulkan
} // namespace gits
