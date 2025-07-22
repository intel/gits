// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "recorder.h"
#include "streams.h"

namespace gits {
namespace DirectX {

class SubcaptureRange {
public:
  SubcaptureRange();

  void frameEnd(bool stateRestore);
  bool isFrameRangeStart();
  void executionStart();
  void executionEnd();
  bool isExecutionRangeStart();
  bool inRange();
  bool commandListSubcapture();

private:
  unsigned executionCount_{};
  unsigned executionRangeStart_{};
  unsigned executionRangeEnd_{};
  bool insideExecution_{};
  bool zeroOrFirstFrame_{true};

  bool inFrameRange_{};
  unsigned startFrame_{};
  unsigned endFrame_{};
  unsigned currentFrame_{};
};

} // namespace DirectX
} // namespace gits
