// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools.h"

#include <mutex>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ContextMapService : public gits::noncopyable {
public:
  ContextMapService() = default;
  ~ContextMapService() = default;

  void setContext(std::uintptr_t context, unsigned key) {
    std::lock_guard<std::mutex> lock(mutex_);
    contextMap_[context] = key;
  }

  unsigned getKey(std::uintptr_t context) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = contextMap_.find(context);
    GITS_ASSERT(it != contextMap_.end());
    return it->second;
  }

  void removeContext(std::uintptr_t context) {
    std::lock_guard<std::mutex> lock(mutex_);
    contextMap_.erase(context);
  }

private:
  std::unordered_map<std::uintptr_t, unsigned> contextMap_{};
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
