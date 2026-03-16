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

class PlaybackOptionsPanel : public BasePanel {
public:
  PlaybackOptionsPanel();

  void Render() override;

  const std::string GetCLIArguments() const;

private:
  void SetScreenshotPathFromInputStream();
  void SetTracePathFromInputStream();

  void ContextCallback(const Event& e);
  void PathCallback(const Event& e);

  struct ExecutableNameConfig {
    bool Enabled = true;
    std::string CustomName = "";
  } ExecutableNameConfig;

  struct HUDConfig {
    bool Enabled = true;
  } HUDConfig;

  struct ScreenshotsConfig {
    bool Enabled = false;
    std::string Range = "";
    int TmpStartFrame = 1;
    int TmpEndFrame = 1;
    int TmpStepFrame = 1;

    const std::string TmpRangeString() {
      if (TmpStepFrame <= 1) {
        return std::to_string(std::min(TmpStartFrame, TmpEndFrame)) + "-" +
               std::to_string(std::max(TmpEndFrame, TmpStartFrame));
      } else {
        return std::to_string(std::min(TmpStartFrame, TmpEndFrame)) + "-" +
               std::to_string(std::max(TmpEndFrame, TmpStartFrame)) + ":" +
               std::to_string(TmpStepFrame);
      }
    }

    bool AddToRange(const std::string& range) {
      // check for already present, optimize string.
      if (Range.empty()) {
        Range = range;
      } else {
        Range += ";" + range;
      }
      return true;
    }

    bool AddTmpRange() {
      return AddToRange(TmpRangeString());
    }

    bool AddTmpFrame() {
      return AddToRange(std::to_string(TmpStartFrame));
    }
  } ScreenshotsConfig;

  struct TraceConfig {
    bool Enabled = false;
  } TraceConfig;
};
} // namespace gits::gui
