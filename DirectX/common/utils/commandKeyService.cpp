// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandKeyService.h"
#include "command.h"

namespace gits {
namespace DirectX {

CommandKeyService::CommandKeyService(const std::string& atCommandKeys) {
  extractKeys(atCommandKeys, commandKeys_);
}

unsigned CommandKeyService::atCommand(unsigned key) {
  auto it = commandKeys_.find(key);
  return (it != commandKeys_.end()) ? *it : 0;
}

std::string CommandKeyService::keyToString(unsigned key) const {
  std::string commandKey;
  if (key & Command::stateRestoreKeyMask) {
    commandKey = "S" + std::to_string(key & ~Command::stateRestoreKeyMask);
  } else {
    commandKey = std::to_string(key);
  }
  return std::move(commandKey);
}

void CommandKeyService::extractKeys(const std::string& keyString,
                                    std::unordered_set<unsigned>& keySet) {
  if (keyString.empty()) {
    return;
  }
  const char* p = keyString.data();
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
      keySet.insert(k);
    } else {
      keySet.insert(std::stoi(key));
    }
  } while (*p++);
}

} // namespace DirectX
} // namespace gits
