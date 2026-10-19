// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tabGroup.h"

#include "basePanel.h"
#include "baseEventBus.h"

namespace gits::gui {

class LiveWorkloadPanel : public BasePanel {
public:
  LiveWorkloadPanel();

  void Render() override;

  float WidthLeftColumn = 0.0f;

private:
  // UI rendering
  void RowTargetPath();
  void RowArguments();
  void RowAPI();
  void RowLaunchAndCleanup();

  // Event callbacks
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
  void ContextCallback(const Event& e);

  CaptureCleanupOptions CleanupOptions;

  std::optional<std::filesystem::path> DroppedFilePath;
  gits::ImGuiHelper::TabGroup<Api> m_APIToolBar;
};

} // namespace gits::gui
