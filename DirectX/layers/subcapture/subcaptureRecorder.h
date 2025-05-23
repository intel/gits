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

class SubcaptureRecorder {
public:
  SubcaptureRecorder();
  ~SubcaptureRecorder();

  SubcaptureRecorder(const SubcaptureRecorder&) = delete;
  SubcaptureRecorder& operator=(const SubcaptureRecorder&) = delete;

  void record(CToken* token);
  void frameEnd();
  void executionStart();
  void executionEnd();
  bool isExecutionRangeStart();
  bool isRunning();

private:
  unsigned executionCount_{};
  unsigned executionRangeStart_{};
  unsigned executionRangeEnd_{};
  bool insideExecution_{};
};

} // namespace DirectX
} // namespace gits
