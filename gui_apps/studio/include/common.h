// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include "baseCommon.h"

namespace gits::gui {

enum class Modules {
  RTSR = 0,
  COUNT
};

enum class SideBarItem {
  LABEL_STREAM = 0,
  METADATA,
  API_TRACE,
  LABEL_LOG,
  LOG,
  APP_LOG,
  COUNT
};

struct AnalysisState {
  bool Screenshots = false;
  bool Trace = false;

  bool FullyAnalysed() {
    return Screenshots && Trace;
  }
};

struct PlayHead {
  uint64_t CurrentFrame = 0;
  uint64_t TotalFrames = 0;

  uint64_t MarkerStart = 0;
  uint64_t MarkerEnd = 0;

  void Init(uint64_t totalFrames) {
    CurrentFrame = 1;
    TotalFrames = totalFrames;
    MarkerStart = 1;
    MarkerEnd = totalFrames;
  }

  float Pos() {
    return TotalFrames == 0
               ? 0.0f
               : std::min(1.0f, static_cast<float>(CurrentFrame) / static_cast<float>(TotalFrames));
  }

  float MarkerStartPos() {
    return TotalFrames == 0 ? 0.0f : (float)MarkerStart / (float)TotalFrames;
  }

  float MarkerEndPos() {
    return TotalFrames == 0 ? 0.0f : (float)MarkerEnd / (float)TotalFrames;
  }

  void SetMarkerStart(uint64_t frame) {
    MarkerStart = std::min(frame, TotalFrames);
  }

  void SetMarkerEnd(uint64_t frame) {
    MarkerEnd = std::min(frame, TotalFrames);
  }

private:
};

} // namespace gits::gui
