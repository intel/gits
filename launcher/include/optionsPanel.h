// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "pluginsPanel.h"
#include "captureOptionsPanel.h"
#include "playbackOptionsPanel.h"
#include "subcaptureOptionsPanel.h"
#ifdef _WIN32
#include "systemSetupPanel.h"
#endif

namespace gits::gui {

class OptionsPanel : public BasePanel {
public:
  OptionsPanel();
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  void Render() override;

private:
  CaptureOptionsPanel m_CaptureOptionsPanel;
  PlaybackOptionsPanel m_PlaybackOptionsPanel;
  SubcaptureOptionsPanel m_SubcaptureOptionsPanel;

  PluginsPanel m_PluginsPanel;
#ifdef _WIN32
  SystemSetupPanel m_SystemSetupPanel;
#endif
};

} // namespace gits::gui
