// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "keyUtils.h"

#include <limits>
#include <stdexcept>

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
        unsigned long parsed = std::stoul(key.substr(1));
        if (parsed > (std::numeric_limits<unsigned>::max() >> 2)) {
          throw std::out_of_range("Config key value out of range: " + key);
        }
        unsigned k = static_cast<unsigned>(parsed);
        if (key[0] == 'S') {
          k |= STATE_RESTORE_KEY_MASK;
        } else if (key[0] == 'E') {
          k |= EXECUTION_SERIALIZATION_KEY_MASK;
        }
        m_CommandKeys.insert(k);
      } else {
        m_CommandKeys.insert(std::stoi(key));
      }
    } while (*p++);
  }
}

std::string ParseConfigKeys(const std::string& keys) {
  std::string result;
  for (size_t i = 0; i < keys.size(); ++i) {
    char c = keys[i];
    if (c == 'S' || c == 'E') {
      const char* begin = &keys[++i];
      while (i < keys.size() && std::isdigit(keys[i])) {
        ++i;
      }
      unsigned long parsed = std::stoul(std::string(begin, &keys[i--]));
      if (parsed > (std::numeric_limits<unsigned>::max() >> 2)) {
        throw std::out_of_range("Config key value out of range");
      }
      unsigned k = static_cast<unsigned>(parsed);
      if (c == 'S') {
        k |= STATE_RESTORE_KEY_MASK;
      } else if (c == 'E') {
        k |= EXECUTION_SERIALIZATION_KEY_MASK;
      }
      result += std::to_string(k);
    } else {
      result += c;
    }
  }
  return result;
}

} // namespace DirectX
} // namespace gits
