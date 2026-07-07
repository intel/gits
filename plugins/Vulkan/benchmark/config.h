// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace gits {
namespace vulkan {

struct BenchmarkConfig {
  std::string Output{"benchmark.csv"};
  // Number of leading presents excluded from the average-FPS summary (warm-up).
  unsigned WarmupFrames{3};
  bool IsCapture{};
};

} // namespace vulkan
} // namespace gits
