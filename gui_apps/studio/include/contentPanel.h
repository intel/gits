// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "basePanel.h"

#include "imGui.h"

#include "common.h"
#include "tabGroup.h"
#include "studioEventBus.h"
#include "metaDataPanel.h"
#include "tracePanel.h"

namespace gits::gui {

class ContentPanel : public BasePanel {
public:
  ContentPanel();
  ~ContentPanel();
  void Render() override;

  float WidthLeftColumn = 0.0f;

private:
  ImGuiHelper::TabGroup<SideBarItem> m_BtnsSideBar;
  MetaDataPanel m_MetaDataPanel;
  TracePanel m_TracePanel;

  void ShowSidebarPanel(SideBarItem panel);

  void StudioEventCallback(const Event& e);
  void AppEventCallback(const Event& e);
  void ActionCallback(const Event& e);
  void NavigateCallback(const Event& e);
};
} // namespace gits::gui
