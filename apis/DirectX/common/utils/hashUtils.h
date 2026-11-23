// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

struct UnsignedPairHash {
  std::size_t operator()(const std::pair<unsigned, unsigned>& p) const {
    return (static_cast<std::size_t>(p.first) << 32) | p.second;
  }
};

} // namespace DirectX
} // namespace gits
