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

struct KeyOffsetHash {
  std::size_t operator()(const std::pair<unsigned long long, unsigned long long>& p) const {
    std::size_t seed = std::hash<unsigned long long>{}(p.first);
    seed ^= std::hash<unsigned long long>{}(p.second) + 0x9e3779b97f4a7c15ull + (seed << 6) +
            (seed >> 2);
    return seed;
  }
};

} // namespace DirectX
} // namespace gits
