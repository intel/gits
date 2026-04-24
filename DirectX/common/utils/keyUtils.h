// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

inline constexpr unsigned STATE_RESTORE_KEY_MASK = 1u << 31;
inline constexpr unsigned EXECUTION_SERIALIZATION_KEY_MASK = 1u << 30;

inline bool IsStateRestoreKey(unsigned key) {
  return key & STATE_RESTORE_KEY_MASK;
}

inline unsigned ExtractStateRestoreKey(unsigned key) {
  return key & ~STATE_RESTORE_KEY_MASK;
}

inline bool IsExecutionSerializationKey(unsigned key) {
  return key & EXECUTION_SERIALIZATION_KEY_MASK;
}

inline unsigned ExtractExecutionSerializationKey(unsigned key) {
  return key & ~EXECUTION_SERIALIZATION_KEY_MASK;
}

std::string ParseConfigKeys(const std::string& keys);

class ConfigKeySet {
public:
  ConfigKeySet(const std::string& keys);
  bool Empty() const {
    return m_CommandKeys.empty();
  }
  bool Contains(unsigned key) const {
    return m_CommandKeys.find(key) != m_CommandKeys.end();
  }
  auto begin() const {
    return m_CommandKeys.begin();
  }
  auto end() const {
    return m_CommandKeys.end();
  }

private:
  std::unordered_set<unsigned> m_CommandKeys;
};

} // namespace DirectX
} // namespace gits
