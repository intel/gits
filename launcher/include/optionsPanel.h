// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "eventBus.h"
#include "textEditorWidget.h"
#include "metaDataPanel.h"
#include "pluginsPanel.h"
#include "captureOptionsPanel.h"
#include "playbackOptionsPanel.h"
#include "subcaptureOptionsPanel.h"

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
};

} // namespace gits::gui
