// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "eventPendingSignalService.h"

namespace gits {
namespace vulkan {

void EventPendingSignalService::MarkPending(VkEvent event) {
  if (event == VK_NULL_HANDLE) {
    return;
  }
  m_HasEventActivity.store(true, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PendingEvents.insert(reinterpret_cast<uint64_t>(event));
}

void EventPendingSignalService::ClearPending(VkEvent event) {
  if (event == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PendingEvents.erase(reinterpret_cast<uint64_t>(event));
}

bool EventPendingSignalService::IsPending(VkEvent event) {
  if (event == VK_NULL_HANDLE) {
    return false;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_PendingEvents.count(reinterpret_cast<uint64_t>(event)) != 0;
}

void EventPendingSignalService::RecordCmdEvent(VkCommandBuffer commandBuffer,
                                               VkEvent event,
                                               bool signaled) {
  if (commandBuffer == VK_NULL_HANDLE || event == VK_NULL_HANDLE) {
    return;
  }
  m_HasEventActivity.store(true, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CommandBufferEventStates[reinterpret_cast<uint64_t>(commandBuffer)]
                            [reinterpret_cast<uint64_t>(event)] = signaled;
}

void EventPendingSignalService::MergeCommandBuffer(VkCommandBuffer primary,
                                                   VkCommandBuffer secondary) {
  // Fast path: skip the lock/lookup on every vkCmdExecuteCommands secondary
  // when no title command has ever touched a VkEvent.
  if (!m_HasEventActivity.load(std::memory_order_relaxed)) {
    return;
  }
  if (primary == VK_NULL_HANDLE || secondary == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_CommandBufferEventStates.find(reinterpret_cast<uint64_t>(secondary));
  if (it == m_CommandBufferEventStates.end()) {
    return;
  }
  auto& primaryStates = m_CommandBufferEventStates[reinterpret_cast<uint64_t>(primary)];
  for (const auto& [eventHandle, signaled] : it->second) {
    primaryStates[eventHandle] = signaled;
  }
}

void EventPendingSignalService::ClearCommandBuffer(VkCommandBuffer commandBuffer) {
  // Fast path: skip the lock/erase on every vkBeginCommandBuffer when no title
  // command has ever touched a VkEvent.
  if (!m_HasEventActivity.load(std::memory_order_relaxed)) {
    return;
  }
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CommandBufferEventStates.erase(reinterpret_cast<uint64_t>(commandBuffer));
}

void EventPendingSignalService::ApplyCommandBuffer(VkCommandBuffer commandBuffer) {
  // Fast path: skip the lock/lookup for every submitted command buffer when no
  // title command has ever touched a VkEvent.
  if (!m_HasEventActivity.load(std::memory_order_relaxed)) {
    return;
  }
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_CommandBufferEventStates.find(reinterpret_cast<uint64_t>(commandBuffer));
  if (it == m_CommandBufferEventStates.end()) {
    return;
  }
  for (const auto& [eventHandle, signaled] : it->second) {
    if (signaled) {
      m_PendingEvents.insert(eventHandle);
    } else {
      m_PendingEvents.erase(eventHandle);
    }
  }
}

void EventPendingSignalService::TrackCommandBuffer(VkCommandBuffer commandBuffer,
                                                   VkCommandPool commandPool) {
  if (commandBuffer == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t cb = reinterpret_cast<uint64_t>(commandBuffer);
  const uint64_t pool = reinterpret_cast<uint64_t>(commandPool);
  m_CommandBufferPool[cb] = pool;
  m_CommandPoolBuffers[pool].insert(cb);
}

void EventPendingSignalService::UntrackCommandBuffer(VkCommandBuffer commandBuffer) {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t cb = reinterpret_cast<uint64_t>(commandBuffer);
  m_CommandBufferEventStates.erase(cb);
  const auto poolIt = m_CommandBufferPool.find(cb);
  if (poolIt == m_CommandBufferPool.end()) {
    return;
  }
  const auto membersIt = m_CommandPoolBuffers.find(poolIt->second);
  if (membersIt != m_CommandPoolBuffers.end()) {
    membersIt->second.erase(cb);
    if (membersIt->second.empty()) {
      m_CommandPoolBuffers.erase(membersIt);
    }
  }
  m_CommandBufferPool.erase(poolIt);
}

void EventPendingSignalService::ResetCommandPool(VkCommandPool commandPool) {
  if (commandPool == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto membersIt = m_CommandPoolBuffers.find(reinterpret_cast<uint64_t>(commandPool));
  if (membersIt == m_CommandPoolBuffers.end()) {
    return;
  }
  for (uint64_t cb : membersIt->second) {
    m_CommandBufferEventStates.erase(cb);
  }
}

void EventPendingSignalService::UntrackCommandPool(VkCommandPool commandPool) {
  if (commandPool == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t pool = reinterpret_cast<uint64_t>(commandPool);
  const auto membersIt = m_CommandPoolBuffers.find(pool);
  if (membersIt == m_CommandPoolBuffers.end()) {
    return;
  }
  for (uint64_t cb : membersIt->second) {
    m_CommandBufferEventStates.erase(cb);
    m_CommandBufferPool.erase(cb);
  }
  m_CommandPoolBuffers.erase(membersIt);
}

} // namespace vulkan
} // namespace gits
