// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include <unordered_map>

namespace gits {
namespace DirectX {

class ContextMapService {
public:
  ContextMapService() = default;
  ~ContextMapService() = default;

  void setContext(unsigned key, std::uintptr_t context) {
    contextMap_[key] = context;
  }

  std::uintptr_t getContext(unsigned key) {
    auto it = contextMap_.find(key);
    GITS_ASSERT(it != contextMap_.end());
    return it->second;
  }

  void removeContext(unsigned key) {
    contextMap_.erase(key);
  }

private:
  std::unordered_map<unsigned, std::uintptr_t> contextMap_{};
};

} // namespace DirectX
} // namespace gits
