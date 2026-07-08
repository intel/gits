// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "optionsPanel.h"

#include "imGui.h"

#include "context.h"
#include "common.h"

namespace gits::gui {

OptionsPanel::OptionsPanel()
    : m_CaptureOptionsPanel(),
      m_PlaybackOptionsPanel(),
      m_SubcaptureOptionsPanel(),
#ifdef WIN32
      m_SystemSetupPanel(),
#endif
      m_PluginsPanel() {
}

void OptionsPanel::Render() {
  if (ImGui::BeginTabBar("OptionsTabs")) {
    if (ImGui::BeginTabItem("Options")) {
      auto& context = Context::GetInstance();
      switch (context.AppMode) {
      case Mode::CAPTURE:
        m_CaptureOptionsPanel.Render();
        break;
      case Mode::PLAYBACK:
        m_PlaybackOptionsPanel.Render();
        break;
      case Mode::SUBCAPTURE:
        m_SubcaptureOptionsPanel.Render();
        break;
      default:
        break;
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Plugins")) {
      m_PluginsPanel.Render();
      ImGui::EndTabItem();
    }
#ifdef WIN32
    if (Context::GetInstance().AppMode == Mode::CAPTURE && ImGui::BeginTabItem("System setup")) {
      m_SystemSetupPanel.Render();
      ImGui::EndTabItem();
    }
#endif
    ImGui::EndTabBar();
  }
}

} // namespace gits::gui
