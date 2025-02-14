// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginUtils.h"
#include "benchmarkUtils.h"

namespace gits {
namespace DirectX {

CGits* g_gits;

void initializeLog(CGits* gits) {
  g_gits = gits;
}

std::string hrToString(HRESULT hr) {
  char s_str[64] = {};
  sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
  return std::string(s_str);
}

void logAndThrow(const std::string& errorMsg) {
  assert(g_gits);
  log(*g_gits, errorMsg);
  throw std::runtime_error(errorMsg);
}

void throwIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    logAndThrow(hrToString(hr));
  }
}

} // namespace DirectX
} // namespace gits
