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
      if (key[0] == 'S') {
        key = key.substr(1);
        unsigned k = std::stoi(key);
        k |= Command::stateRestoreKeyMask;
        commandKeys_.insert(k);
      } else {
        commandKeys_.insert(std::stoi(key));
      }
    } while (*p++);
  }
}

std::string ConfigKeySet::keyToString(unsigned key) {
  std::string commandKey;
  if (key & Command::stateRestoreKeyMask) {
    commandKey = "S" + std::to_string(key & ~Command::stateRestoreKeyMask);
  } else {
    commandKey = std::to_string(key);
  }
  return commandKey;
}

std::wstring ConfigKeySet::keyToWString(unsigned key) {
  std::string commandKey = keyToString(key);
  return std::wstring(commandKey.begin(), commandKey.end());
}

} // namespace DirectX
} // namespace gits
