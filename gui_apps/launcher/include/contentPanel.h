// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "baseEventBus.h"
#include "textEditorWidget.h"
#include "metaDataPanel.h"
#include "optionsPanel.h"
#include "yamlPanel.h"
#include "resourceDumpPanel.h"

namespace gits::gui {

class ContentPanel : public BasePanel {
public:
  ContentPanel();
  ~ContentPanel() override;
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  float WidthColumn1(bool resetSize = false);
  void Render() override;
  void RenderSimple();

private:
  void GITSButton();

  // Event callbacks
  void CaptureActionCallback(const Event& e);

  OptionsPanel m_OptionsPanel;
  YAMLPanel m_YAMLPanel;
  MetaDataPanel m_MetaDataPanel;
  ResourceDumpPanel m_ResourceDumpPanel;
};

} // namespace gits::gui
