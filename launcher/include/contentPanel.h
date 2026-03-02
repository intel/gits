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

namespace gits::gui {

class ContentPanel : public BasePanel {
public:
  ContentPanel();
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  float WidthColumn1(bool resetSize = false);
  void Render() override;

private:
  void ChildWindowConfig();
  void GITSButton();

  // Event callbacks
  void ThemeChangedCallback(const Event& event);
  void CliUpdatedCallback(const Event& event);
  void CaptureActionCallback(const Event& e);

  gits::ImGuiHelper::TextEditorWidget CLIEditor;
  MetaDataPanel m_MetaDataPanel;
};

} // namespace gits::gui
