// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <mutex>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ContextMapService {
public:
  void SetContext(std::uintptr_t context, unsigned key) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ContextMap[context] = key;
  }

  unsigned GetKey(std::uintptr_t context) {
    if (!context) {
      return 0;
    }
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_ContextMap.find(context);
    GITS_ASSERT(it != m_ContextMap.end());
    return it->second;
  }

  void RemoveContext(std::uintptr_t context) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ContextMap.erase(context);
  }

private:
  std::unordered_map<std::uintptr_t, unsigned> m_ContextMap{};
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
