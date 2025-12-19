// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
using IOTAIntRange = std::ranges::iota_view<int, int>;

class EzOptionsPanel : public BasePanel {
public:
  EzOptionsPanel(ISharedContext& sharedContext) : BasePanel(sharedContext) {
    // todo: read in screenshot paths from config/past
  }

  void Render() override;

  const std::string GetCLIArguments() const;

private:
  struct HUDConfig {
    bool Enabled = false;
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
