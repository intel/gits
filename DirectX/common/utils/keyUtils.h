// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

inline constexpr unsigned stateRestoreKeyMask{1u << 31};
inline constexpr unsigned executionSerializationKeyMask{1u << 30};

inline bool isStateRestoreKey(unsigned key) {
  return key & stateRestoreKeyMask;
}

inline unsigned extractStateRestoreKey(unsigned key) {
  return key & ~stateRestoreKeyMask;
}

inline bool isExecutionSerializationKey(unsigned key) {
  return key & executionSerializationKeyMask;
}

inline unsigned extractExecutionSerializationKey(unsigned key) {
  return key & ~executionSerializationKeyMask;
}

class ConfigKeySet {
public:
  ConfigKeySet(const std::string& keys);
  bool empty() {
    return commandKeys_.empty();
  }
  bool contains(unsigned key) {
    return commandKeys_.find(key) != commandKeys_.end();
  }
  auto begin() {
    return commandKeys_.begin();
  }
  auto end() {
    return commandKeys_.end();
  }

private:
  std::unordered_set<unsigned> commandKeys_;
};

} // namespace DirectX
} // namespace gits
