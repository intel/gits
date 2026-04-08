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

  const std::string GetCLIArguments() const;

private:
  void OnPlaybackEnded(const Event& e);

  // Doubles the step in each segment of a range string.
  // Input format: "start-end:step,single,start-end,..."
  // Output: steps are doubled (minimum 2 for ranges that had implicit step of 1).
  std::string DoubleRangeSteps(const std::string& rangeString);

  struct ResourcesDumpConfig {
    bool Enabled = false;
    std::string ResourceKeys;
    std::string CommandKeys;
    std::string TextureRescaleRange;
    int FormatIdx = 0; // 0=PNG, 1=JPEG
  } ResourcesDumpConfig;

  struct RenderTargetsDumpConfig {
    bool Enabled = false;
    bool IncreaseStepOnOutOfMemoryError = true;
    std::string Frames;
    int TmpFrameStart = 1;
    int TmpFrameEnd = 1;
    int TmpFrameStep = 1;
    std::string Draws;
    int TmpDrawStart = 1;
    int TmpDrawEnd = 1;
    int TmpDrawStep = 1;
    int FormatIdx = 0; // 0=PNG, 1=JPEG
    bool DryRun = false;
  } RenderTargetsDumpConfig;

  struct DispatchOutputsDumpConfig {
    bool Enabled = false;
    std::string Frames;
    int TmpFrameStart = 1;
    int TmpFrameEnd = 1;
    int TmpFrameStep = 1;
    std::string Dispatches;
    int TmpDispatchStart = 1;
    int TmpDispatchEnd = 1;
    int TmpDispatchStep = 1;
    int FormatIdx = 0; // 0=PNG, 1=JPEG
    bool DryRun = false;
  } DispatchOutputsDumpConfig;

  struct RaytracingDumpConfig {
    bool Enabled = false;
    bool BindingTablesPre = false;
    bool BindingTablesPost = false;
    bool InstancesPre = false;
    bool InstancesPost = false;
    bool Blases = false;
    std::string CommandKeys;
    std::string CommandListModuloStep;
    int TmpModuloStart = 1;
    int TmpModuloEnd = 1;
    int TmpModuloStep = 1;
  } RaytracingDumpConfig;

  struct ExecuteIndirectDumpConfig {
    bool Enabled = false;
    bool ArgumentBufferPre = false;
    bool ArgumentBufferPost = false;
    std::string CommandKeys;
  } ExecuteIndirectDumpConfig;

  struct RootSignatureDumpConfig {
    bool Enabled = false;
    std::string Keys;
  } RootSignatureDumpConfig;

  int oomRetryCount_ = 0;
  static constexpr int kMaxOomRetries = 5;
};
} // namespace gits::gui
