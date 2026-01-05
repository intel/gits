// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "toStr.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

std::string toStr(const wchar_t* s) {
  // Convert wide string to narrow string by truncating the characters
  // Note: Will not work for non-ASCII characters
  std::wstring wStr = s;
  return std::string(wStr.begin(), wStr.end());
}

std::string keyToStr(unsigned key) {
  std::string result;
  if (isStateRestoreKey(key)) {
    result += 'S';
    key = extractStateRestoreKey(key);
  } else if (isExecutionSerializationKey(key)) {
    result += 'E';
    key = extractExecutionSerializationKey(key);
  }
  return result + std::to_string(key);
}

std::wstring keyToWStr(unsigned key) {
  std::string keyStr = keyToStr(key);
  return std::wstring(keyStr.begin(), keyStr.end());
}

} // namespace DirectX
} // namespace gits
