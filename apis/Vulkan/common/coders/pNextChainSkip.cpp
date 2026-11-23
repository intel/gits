// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pNextChainSkip.h"

#include <mutex>
#include <unordered_set>

#include "log.h"

namespace gits {
namespace vulkan {

bool ShouldSkipPNext(VkStructureType sType) {
  switch (sType) {
  default:
    return false;
  }
}

void LogSkippedPNext(const char* funcName, VkStructureType sType) {
  static std::mutex mutex;
  static std::unordered_set<uint32_t> alreadyLogged;
  const uint32_t key = static_cast<uint32_t>(sType);
  std::lock_guard<std::mutex> lock(mutex);
  if (alreadyLogged.insert(key).second) {
    LOG_INFO << funcName << ": skipping pNext structure (sType: " << key << ")";
  }
}

} // namespace vulkan
} // namespace gits
