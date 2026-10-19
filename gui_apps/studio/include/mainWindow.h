// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "studioEventBus.h"

#include <memory>

#include "contentPanel.h"
#include "liveWorkloadPanel.h"
#include "streamPanel.h"

namespace gits::gui {

class MainWindow : BasePanel {
public:
  MainWindow();

  ~MainWindow();

  void Render() override;
  void GITSBaseRow();
  void RenderMainMenu();

  float WidthLeftColumn = 0.0f;

private:
  void PathCallback(const Event& e);

  StreamPanel m_StreamPanel;
  LiveWorkloadPanel m_LiveWorkloadPanel;
  ContentPanel m_ContentPanel;

  bool m_ShowReleaseNotes = false;
};

} // namespace gits::gui
