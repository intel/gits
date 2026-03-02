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
#include "eventBus.h"

namespace gits::gui {

class MainWindow : BasePanel {
public:
  MainWindow();

  ~MainWindow();

  void Render() override;

  float WidthLeftColumn = 0.0f;

  const std::string GetCLIArguments() const;
  // TODO: Move this once we have messages in
  const CapturePanel::CaptureCleanupOptions GetCleanupOptions() const;

  void SetPlaybackFile(const std::filesystem::path& filePath);

private:
  std::unique_ptr<ContentPanel> contentPanel;
  std::unique_ptr<PlaybackPanel> playbackPanel;
  std::unique_ptr<CapturePanel> capturePanel;
  std::unique_ptr<SubcapturePanel> subcapturePanel;
  std::unique_ptr<gits::ImGuiHelper::TabGroup<Mode>> tabsToolBar;

  bool m_CaptureInProgress = false;

  void GITSButton();
  void ModeSelectionButtons();
  void MainActionButton();

  void GITSBaseRow();
  void GITSPlayerRow();

  // Event handlers
  void CaptureActionCallback(const Event& e);
};

} // namespace gits::gui
