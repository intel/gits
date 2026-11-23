// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "fencePendingSignalService.h"

namespace gits {
namespace vulkan {

void FencePendingSignalService::MarkPending(VkFence fence) {
  if (fence == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PendingFences.insert(reinterpret_cast<uint64_t>(fence));
}

void FencePendingSignalService::ClearPending(VkFence fence) {
  if (fence == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PendingFences.erase(reinterpret_cast<uint64_t>(fence));
}

bool FencePendingSignalService::IsPending(VkFence fence) {
  if (fence == VK_NULL_HANDLE) {
    return false;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_PendingFences.count(reinterpret_cast<uint64_t>(fence)) != 0;
}

} // namespace vulkan
} // namespace gits
