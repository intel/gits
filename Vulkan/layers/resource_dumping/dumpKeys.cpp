// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dumpKeys.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace gits {
namespace vulkan {

namespace {

std::string Trim(const std::string& text) {
  const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
  auto begin = std::find_if_not(text.begin(), text.end(), isSpace);
  auto end = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
  return begin < end ? std::string(begin, end) : std::string();
}

GITSKey ParseKey(const std::string& token) {
  size_t pos = 0;
  if (token[pos] == 'O') {
    ++pos;
  }
  bool stateRestore = false;
  if (pos < token.size() && token[pos] == 'S') {
    stateRestore = true;
    ++pos;
  }
  const std::string digits = token.substr(pos);
  if (digits.empty() || !std::all_of(digits.begin(), digits.end(),
                                     [](unsigned char c) { return std::isdigit(c) != 0; })) {
    throw std::invalid_argument("Invalid key '" + token +
                                "': expected a decimal key with an optional O or S prefix");
  }
  GITSKey key{};
  try {
    key = std::stoull(digits);
  } catch (const std::out_of_range&) {
    throw std::invalid_argument("Key '" + token + "' is out of range");
  }
  if (IsStateRestoreKey(key)) {
    throw std::invalid_argument("Key '" + token + "' is out of range");
  }
  return stateRestore ? key | STATE_RESTORE_KEY_MASK : key;
}

} // namespace

std::unordered_set<GITSKey> ParseDumpKeys(const std::string& keys) {
  std::unordered_set<GITSKey> result;
  size_t begin = 0;
  while (begin <= keys.size()) {
    size_t end = keys.find(',', begin);
    if (end == std::string::npos) {
      end = keys.size();
    }
    const std::string token = Trim(keys.substr(begin, end - begin));
    if (!token.empty()) {
      result.insert(ParseKey(token));
    }
    begin = end + 1;
  }
  return result;
}

std::string DumpKeyToString(GITSKey key) {
  if (IsStateRestoreKey(key)) {
    return "S" + std::to_string(ExtractStateRestoreKey(key));
  }
  return std::to_string(key);
}

} // namespace vulkan
} // namespace gits
