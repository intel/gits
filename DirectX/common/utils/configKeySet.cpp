// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configKeySet.h"
#include "command.h"

namespace gits {
namespace DirectX {

ConfigKeySet::ConfigKeySet(const std::string& keys) {
  if (!keys.empty()) {
    const char* p = keys.data();
    do {
      const char* begin = p;
      while (*p != ',' && *p) {
        ++p;
      }
      std::string key(begin, p);
      if (key[0] == 'S' || key[0] == 'E') {
        key = key.substr(1);
        unsigned k = std::stoi(key);
        if (key[0] == 'S') {
          k |= Command::stateRestoreKeyMask;
        } else if (key[0] == 'E') {
          k |= Command::executionSerializationKeyMask;
        }
        commandKeys_.insert(k);
      } else {
        commandKeys_.insert(std::stoi(key));
      }
    } while (*p++);
  }
}

} // namespace DirectX
} // namespace gits
