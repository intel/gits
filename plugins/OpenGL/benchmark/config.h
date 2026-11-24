// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace gits::opengl {

struct BenchmarkConfig {
  std::string Output{"benchmark.csv"};
  // Number of leading frames excluded from the average-FPS summary (warm-up).
  unsigned WarmupFrames{3};
  bool IsCapture{};
  // Under the recorder, whether it records a stream.
  bool IsRecording{};
};

} // namespace gits::opengl
