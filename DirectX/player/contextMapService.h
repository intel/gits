// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class ContextMapService {
public:
  ContextMapService() = default;
  ~ContextMapService() = default;

  void SetContext(unsigned key, std::uintptr_t context) {
    m_ContextMap[key] = context;
  }

  std::uintptr_t GetContext(unsigned key) {
    if (!key) {
      return {};
    }
    auto it = m_ContextMap.find(key);
    GITS_ASSERT(it != m_ContextMap.end());
    return it->second;
  }

  void RemoveContext(unsigned key) {
    m_ContextMap.erase(key);
  }

private:
  std::unordered_map<unsigned, std::uintptr_t> m_ContextMap{};
};

} // namespace DirectX
} // namespace gits
