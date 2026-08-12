// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <windows.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class FenceTrackingService {
public:
  void SetFenceValue(GITSKey fenceKey, UINT64 fenceValue) {
    m_FenceValues[fenceKey] = fenceValue;
  }
  UINT64 GetFenceValue(GITSKey fenceKey) {
    return m_FenceValues[fenceKey];
  }

private:
  std::unordered_map<GITSKey, UINT64> m_FenceValues;
};

} // namespace DirectX
} // namespace gits
