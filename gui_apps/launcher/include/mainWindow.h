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

#include "context.h"
#include "contentPanel.h"
#include "playbackPanel.h"
#include "capturePanel.h"
#include "baseEventBus.h"
#include "launcherActions.h"

namespace gits::gui {

class MainWindow : BasePanel {
public:
  MainWindow();

  ~MainWindow();

  void Render() override;

  float WidthLeftColumn = 0.0f;

  const std::string GetCLIArguments() const;
  // TODO: Move this once we have messages in
  const CaptureCleanupOptions GetCleanupOptions() const;

  void SetPlaybackFile(const std::filesystem::path& filePath);

private:
  std::unique_ptr<ContentPanel> contentPanel;
  std::unique_ptr<PlaybackPanel> playbackPanel;
  std::unique_ptr<CapturePanel> capturePanel;
  std::unique_ptr<gits::ImGuiHelper::TabGroup<GitsAction>> tabsToolBar;

  bool m_CaptureInProgress = false;
  bool m_ShowReleaseNotes = false;
  bool m_ShowCCodeGeneration = false;
  CCodeExport m_CCodeParameters;

  void FileMenu();
  void HelpMenu();
  void ModeSelectionButtons();
  void ModeToggle();
  void MainActionButtons();
  void ShowReleaseNotesModal();
  void ShowCCodeModal();

  void SimpleCurrentActionLabel();
  void SimpleActionButtons();
  void RenderSimpleMode();
  void RenderSimpleModeSelection();

  void RenderAdvancedMode();

  std::optional<GitsAction> VisibleAction() const;

  void GITSBaseRow();

  // Event handlers
  void CaptureActionCallback(const Event& e);
  void PathCallback(const Event& e);
};

} // namespace gits::gui
