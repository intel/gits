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
  void record(CToken* token);
  void frameEnd();
  bool isRunning();

private:
  bool initialized_{false};
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
