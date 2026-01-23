// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"

#include <memory>

#include "tabGroup.h"

#include "Context.h"
#include "contentPanel.h"
#include "playbackPanel.h"
#include "capturePanel.h"
#include "subcapturePanel.h"

namespace gits::gui {

class MainWindow : gits::gui::BasePanel {
public:
  enum class MODE_BUTTON_ITEMS {
    PLAYBACK = 0,
    CAPTURE,
    SUBCAPTURE
  };

  MainWindow(gits::gui::ISharedContext& sharedContext);

  ~MainWindow();

  void Render() override;

  float WidthLeftColumn = 0.0f;

  const std::string GetCLIArguments() const;
  // TODO: Move this once we have messages in
  const CapturePanel::CaptureCleanupOptions GetCleanupOptions() const;

private:
  std::unique_ptr<gits::gui::ContentPanel> contentPanel;
  std::unique_ptr<gits::gui::PlaybackPanel> playbackPanel;
  std::unique_ptr<gits::gui::CapturePanel> capturePanel;
  std::unique_ptr<gits::gui::SubcapturePanel> subcapturePanel;
  std::unique_ptr<gits::ImGuiHelper::TabGroup<MODE_BUTTON_ITEMS>> tabsToolBar;

  void GITSButton();
  void ModeSelectionButtons();
  void MainActionButton();

  void GITSBaseRow();
  void GITSPlayerRow();
};

} // namespace gits::gui
