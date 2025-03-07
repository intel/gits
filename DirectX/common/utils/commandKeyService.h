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

class CommandKeyService {
public:
  CommandKeyService(const std::string& keys);
  bool contains(unsigned key) {
    return commandKeys_.find(key) != commandKeys_.end();
  }
  static std::string keyToString(unsigned key);

private:
  std::unordered_set<unsigned> commandKeys_;
};

} // namespace DirectX
} // namespace gits
