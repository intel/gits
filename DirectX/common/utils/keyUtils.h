// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

inline constexpr GITSKey STATE_RESTORE_KEY_MASK = GITSKey(1u << 31);
inline constexpr GITSKey EXECUTION_SERIALIZATION_KEY_MASK = GITSKey(1u << 30);

inline bool IsStateRestoreKey(GITSKey key) {
  return key & STATE_RESTORE_KEY_MASK;
}

inline GITSKey ExtractStateRestoreKey(GITSKey key) {
  return key & ~STATE_RESTORE_KEY_MASK;
}

inline bool IsExecutionSerializationKey(GITSKey key) {
  return key & EXECUTION_SERIALIZATION_KEY_MASK;
}

inline GITSKey ExtractExecutionSerializationKey(GITSKey key) {
  return key & ~EXECUTION_SERIALIZATION_KEY_MASK;
}

std::string ParseConfigKeys(const std::string& keys);

class ConfigKeySet {
public:
  ConfigKeySet(const std::string& keys);
  bool Empty() const {
    return m_CommandKeys.empty();
  }
  bool Contains(GITSKey key) const {
    return m_CommandKeys.find(key) != m_CommandKeys.end();
  }
  auto begin() const {
    return m_CommandKeys.begin();
  }
  auto end() const {
    return m_CommandKeys.end();
  }

private:
  std::unordered_set<GITSKey> m_CommandKeys;
};

} // namespace DirectX
} // namespace gits
