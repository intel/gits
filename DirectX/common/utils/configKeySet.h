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
  static std::string keyToString(unsigned key);
  static std::wstring keyToWString(unsigned key);

private:
  std::unordered_set<unsigned> commandKeys_;
};

} // namespace DirectX
} // namespace gits
