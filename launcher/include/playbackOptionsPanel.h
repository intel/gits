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

private:
  void SetScreenshotPathFromInputStream();

  void ContextCallback(const Event& e);
  void PathCallback(const Event& e);

  struct ScreenshotsConfig {
    int TmpStartFrame = 1;
    int TmpEndFrame = 1;
    int TmpStepFrame = 1;
  } ScreenshotsConfig;
};
} // namespace gits::gui
