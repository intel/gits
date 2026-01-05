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

#include "Context.h"
#include "contentPanel.h"
#include "playbackPanel.h"
#include "capturePanel.h"
#include "subcapturePanel.h"

namespace gits::gui {

class MainWindow : gits::gui::BasePanel {
public:
  MainWindow(gits::gui::ISharedContext& sharedContext);

  ~MainWindow();

  void Render() override;

  float WidthLeftColumn = 0.0f;

  const std::string GetCLIArguments() const;

private:
  std::unique_ptr<gits::gui::ContentPanel> contentPanel;
  std::unique_ptr<gits::gui::PlaybackPanel> playbackPanel;
  std::unique_ptr<gits::gui::CapturePanel> capturePanel;
  std::unique_ptr<gits::gui::SubcapturePanel> subcapturePanel;

  void GITSButton();
  void ModeSelectionButtons();
  void MainActionButton();

  void GITSBaseRow();
  void GITSPlayerRow();
};

} // namespace gits::gui
