// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "moduleBasePanel.h"
#include "rtsr/ui/rtsrEvents.h"
#include "rtsr/ui/rtsrModule.h"
#include "rtsr/ui/rtsrPerformancePanel.h"
#include "rtsr/ui/rtsrShaderPanel.h"

#include <optional>
#include <string>
#include <vector>

// Forward declared rather than including implot.h, which would drag ImPlot's
// headers into everything that includes this panel.
struct ImPlotContext;

namespace gits::gui {

// Root panel of the RTSR module. It runs the RTSR server, names the GITS plugin
// the host adds to every run it launches while the module is enabled, and hosts
// the two tabs: Shaders and Performance.
//
// It owns the module's single event subscription and fans events out to both
// tabs, so the one that is not on screen stays current.
class RtsrPanel : public ModuleBasePanel {
public:
  RtsrPanel();
  ~RtsrPanel() override;

  void Render() override;

  ModuleId GetID() const override;
  std::string GetName() const override;
  std::string GetNameHint() const override;

  bool Activate() override;
  bool Deactivate() override;
  bool IsActive() override;

  std::vector<std::string> Plugins(Api api) const override;

private:
  void OnRtsrEvent(const RtsrEvent& event);
  // The text editors take their colours from the ImGui style once, so they have
  // to be told when the style changes.
  void OnThemeChanged();

  bool m_Enabled{false};
  // Why enabling the module failed. Outlives the checkbox turning itself back
  // off, which is the only way the user gets to read it.
  std::string m_Notice;
  // Studio mode at the moment the client attached. Empty while no session is
  // open. Used only to warn when the user switches Live/Stream while a session
  // from the other mode is still attached.
  std::optional<GitsAction> m_SessionAction;

  // Declared before the tabs, which hold references to it.
  RtsrModule m_Module;
  RtsrShaderPanel m_ShaderPanel;
  RtsrPerformancePanel m_PerformancePanel;

  // Raised when the performance tab asks for a pipeline to be shown, consumed
  // by the next tab bar render.
  bool m_SwitchToShaders{false};

  // ImPlot keeps a context of its own, which has to be created after ImGui's and
  // destroyed before it. Owned here rather than by the application because the
  // performance view is the only thing in GITS that plots, and the ordering
  // holds: ImGuiApp::Shutdown destroys the panels through DestroyUI() before
  // VKWindow::CleanUpImGUI destroys ImGui's context.
  ImPlotContext* m_PlotContext{};
};

} // namespace gits::gui
