// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <ranges>

#include <filesystem>

#include "basePanel.h"

namespace gits::gui {
struct Event;

using IOTAIntRange = std::ranges::iota_view<int, int>;

class ResourceDumpPanel : public BasePanel {
public:
  ResourceDumpPanel();

  void Render() override;

private:
  void OnPlaybackEnded(const Event& e);

  // Doubles the step in each segment of a range string.
  // Input format: "start-end:step,single,start-end,..."
  // Output: steps are doubled (minimum 2 for ranges that had implicit step of 1).
  std::string DoubleRangeSteps(const std::string& rangeString);

  struct ResourcesDumpConfig {
    int FormatIdx = 0; // 0=PNG, 1=JPEG
  } ResourcesDumpConfig;

  struct RenderTargetsDumpConfig {
    bool IncreaseStepOnOutOfMemoryError = true;
    int TmpFrameStart = 1;
    int TmpFrameEnd = 1;
    int TmpFrameStep = 1;

    int TmpDrawStart = 1;
    int TmpDrawEnd = 1;
    int TmpDrawStep = 1;
    int FormatIdx = 0; // 0=PNG, 1=JPEG
  } RenderTargetsDumpConfig;

  struct DispatchOutputsDumpConfig {
    int TmpFrameStart = 1;
    int TmpFrameEnd = 1;
    int TmpFrameStep = 1;

    int TmpDispatchStart = 1;
    int TmpDispatchEnd = 1;
    int TmpDispatchStep = 1;
    int FormatIdx = 0; // 0=PNG, 1=JPEG
  } DispatchOutputsDumpConfig;

  struct RaytracingDumpConfig {
    bool Enabled = false;
    int TmpModuloStart = 1;
    int TmpModuloEnd = 1;
    int TmpModuloStep = 1;
  } RaytracingDumpConfig;

  struct ExecuteIndirectDumpConfig {
    bool Enabled = false;
  } ExecuteIndirectDumpConfig;

  int oomRetryCount_ = 0;
  static constexpr int kMaxOomRetries = 5;
};
} // namespace gits::gui
