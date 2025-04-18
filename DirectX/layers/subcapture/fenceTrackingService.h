// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class FenceTrackingService {
public:
  void setFenceValue(unsigned fenceKey, UINT64 fenceValue) {
    fenceValues_[fenceKey] = fenceValue;
  }
  UINT64 getFenceValue(unsigned fenceKey) {
    return fenceValues_[fenceKey];
  }

private:
  std::unordered_map<unsigned, UINT64> fenceValues_;
};

} // namespace DirectX
} // namespace gits
