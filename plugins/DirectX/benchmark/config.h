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
namespace DirectX {

struct BenchmarkConfig {
  struct CpuFrameBenchmarkConfig {
    bool enabled{false};
    std::string output{"benchmarkCPUPerFrame.csv"};
  } cpuFrameBenchmarkConfig;
};

} // namespace DirectX
} // namespace gits
