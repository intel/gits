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
    std::array<UINT64, 3>& values = fenceValues_[fenceKey];
    values[2] = values[1];
    values[1] = values[0];
    values[0] = fenceValue;
  }
  std::array<UINT64, 3> getFenceValue(unsigned fenceKey) {
    return fenceValues_[fenceKey];
  }
  bool incremental(std::array<UINT64, 3>& values) {
    return values[2] <= values[1] && values[1] <= values[0];
  }

private:
  std::unordered_map<unsigned, std::array<UINT64, 3>> fenceValues_;
};

} // namespace DirectX
} // namespace gits
