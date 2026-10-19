// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrPanel.h"
#include "rtsr/ui/rtsrLabels.h"
#include "rtsr/shared/rtsrCommon.h"

#include "moduleRegistry.h"
#include "baseEventBus.h"
#include "baseContext.h"
#include "log.h"

#include <imgui.h>
#include <implot.h>

GITS_GUI_REGISTER_MODULE(Rtsr,
                         ::gits::gui::RtsrPanel,
                         ::gits::rtsr::MODULE_NAME,
                         ::gits::rtsr::MODULE_NAME_HINT)

namespace gits::gui {

namespace {

using LB = RtsrLabels;

} // namespace

RtsrPanel::RtsrPanel() : m_ShaderPanel(m_Module), m_PerformancePanel(m_Module) {
  // Safe here: panels are built from ImGuiApp::Initialize, after the window has
  // created ImGui's context.
  m_PlotContext = ImPlot::CreateContext();

  m_PerformancePanel.SetGoToPipeline([this](rtsr::RTSR_ID id) {
    m_ShaderPanel.RevealPipeline(id);
    m_SwitchToShaders = true;
  });

  // The theme is applied before the panels are built, so this picks up whichever
  // one the application started in.
  OnThemeChanged();

  // Subscribed once, for the panel's whole life. EventBus::unsubscribe cannot
  // match a lambda, so subscribing per activation would pile up handlers that
  // can never be removed.
  EventBus::GetInstance().subscribe<RtsrEvent>(
      [this](const Event& event) { OnRtsrEvent(static_cast<const RtsrEvent&>(event)); });
  EventBus::GetInstance().subscribe<AppEvent>([this](const Event&) { OnThemeChanged(); },
                                              {AppEvent::Type::ThemeChanged});
}

RtsrPanel::~RtsrPanel() {
  // The server has to stop before the members its callbacks reach are gone, and
  // while winsock is still loaded. The event bus outlives this panel and still
  // holds both subscriptions above, which is only safe because the panel lives
  // as long as the application does.
  m_Module.Stop();

  if (m_PlotContext != nullptr) {
    ImPlot::DestroyContext(m_PlotContext);
    m_PlotContext = nullptr;
  }
}

void RtsrPanel::OnThemeChanged() {
  // Only the Shaders tab holds text editors. The plots read the ImGui style
  // every frame, so they follow the theme with nothing to invalidate.
  m_ShaderPanel.OnThemeChanged();
}

void RtsrPanel::OnRtsrEvent(const RtsrEvent& event) {
  // The bus is drained a frame after it is written, so an event can outlive the
  // server that raised it. One raised just before the module was disabled
  // describes a run both tabs have already discarded, and applying it would put
  // a finished run back on a panel that is meant to be waiting.
  if (!m_Module.IsListening()) {
    return;
  }

  if (event.EventType == RtsrEvent::Type::SessionOpened) {
    // Stamp the Studio mode that is active when the client attaches. Do not
    // compare ApplicationPath to gitsPlayer.exe: ExecutableNameOverride copies
    // the player under the captured app's name, so that path is never the
    // player's and a Stream run would be mis-labelled as Live.
    m_SessionAction = BaseContext::GetBaseInstance().SelectedAction;
  } else if (event.EventType == RtsrEvent::Type::SessionClosed) {
    m_SessionAction.reset();
  }

  // Both tabs see every event, so the one that is not on screen does not fall
  // behind and need rebuilding when the user switches to it.
  m_ShaderPanel.OnRtsrEvent(event);
  m_PerformancePanel.OnRtsrEvent(event);
}

ModuleId RtsrPanel::GetID() const {
  return ModuleId::Rtsr;
}

std::string RtsrPanel::GetName() const {
  return rtsr::MODULE_NAME;
}

std::string RtsrPanel::GetNameHint() const {
  return rtsr::MODULE_NAME_HINT;
}

std::vector<std::string> RtsrPanel::Plugins(Api api) const {
  if (api == Api::DIRECTX || api == Api::VULKAN) {
    return {rtsr::PLUGIN_NAME};
  }
  return {};
}

bool RtsrPanel::Activate() {
  m_Notice.clear();
  m_Enabled = m_Module.Start();
  if (!m_Enabled) {
    LOG_ERROR << LB::PORTS_TAKEN;
    m_Notice = LB::PORTS_TAKEN;
  }
  return m_Enabled;
}

bool RtsrPanel::Deactivate() {
  const bool wasEnabled = m_Enabled;
  m_Module.Stop();

  // Disabling the module discards what it harvested; a run merely ending does
  // not. Done here rather than through an event because this already runs on
  // the UI thread and owns both tabs.
  m_ShaderPanel.ClearSession();
  m_PerformancePanel.ClearSession();
  m_SessionAction.reset();

  m_Enabled = false;
  return wasEnabled;
}

bool RtsrPanel::IsActive() {
  return m_Enabled;
}

void RtsrPanel::Render() {
  if (!m_Notice.empty()) {
    ImGui::TextUnformatted(m_Notice.c_str());
  }

  if (!m_Enabled) {
    ImGui::TextUnformatted(LB::DISABLED_HINT);
    return;
  }

  // Only one process can be attached at a time, so nothing launched from this
  // mode is instrumented until the attached one exits.
  if (m_SessionAction.has_value() &&
      *m_SessionAction != BaseContext::GetBaseInstance().SelectedAction &&
      m_Module.IsSessionOpen()) {
    ImGui::TextWrapped("%s", LB::SESSION_ATTACHED_ELSEWHERE);
  }

  if (!ImGui::BeginTabBar("RtsrTabs")) {
    return;
  }

  ImGuiTabItemFlags shaderFlags = ImGuiTabItemFlags_None;
  if (m_SwitchToShaders) {
    m_SwitchToShaders = false;
    shaderFlags = ImGuiTabItemFlags_SetSelected;
  }

  if (ImGui::BeginTabItem(LB::TAB_PIPELINES, nullptr, shaderFlags)) {
    m_ShaderPanel.Render();
    ImGui::EndTabItem();
  }
  if (ImGui::BeginTabItem(LB::TAB_PERFORMANCE)) {
    m_PerformancePanel.Render();
    ImGui::EndTabItem();
  }

  ImGui::EndTabBar();
}

} // namespace gits::gui
