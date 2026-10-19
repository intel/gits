// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "optionsPanel.h"

#include "imgui.h"

#include "context.h"
#include "baseContext.h"
#include "baseCommon.h"

namespace {
bool ArePluginsSupported() {
  auto& context = gits::gui::Context::GetInstance();
  const auto api = context.SelectedAction == gits::gui::GitsAction::CAPTURE
                       ? context.SelectedApiForCapture
                       : context.GetStreamAPI();
  return api == gits::gui::Api::DIRECTX || api == gits::gui::Api::VULKAN;
};
} // namespace

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
  auto& baseContext = gits::gui::BaseContext::GetBaseInstance();

  if (ImGui::BeginTabBar("OptionsTabs")) {
    if (ImGui::BeginTabItem("Options")) {
      switch (baseContext.SelectedAction) {
      case GitsAction::CAPTURE:
        m_CaptureOptionsPanel.Render();
        break;
      case GitsAction::PLAYBACK:
        m_PlaybackOptionsPanel.Render();
        break;
      case GitsAction::SUBCAPTURE:
        m_SubcaptureOptionsPanel.Render();
        break;
      default:
        break;
      }
      ImGui::EndTabItem();
    }
    if (ArePluginsSupported() && ImGui::BeginTabItem("Plugins")) {
      m_PluginsPanel.Render();
      ImGui::EndTabItem();
    }
#ifdef WIN32
    if (baseContext.SelectedAction == GitsAction::CAPTURE && ImGui::BeginTabItem("System setup")) {
      m_SystemSetupPanel.Render();
      ImGui::EndTabItem();
    }
#endif
    ImGui::EndTabBar();
  }
}

void OptionsPanel::RenderSimple() {
  auto& context = Context::GetInstance();

  switch (context.SelectedAction) {
  case GitsAction::CAPTURE:
    m_CaptureOptionsPanel.RenderSimple();
    break;
  case GitsAction::PLAYBACK:
    m_PlaybackOptionsPanel.RenderSimple();
    break;
  case GitsAction::SUBCAPTURE:
    m_SubcaptureOptionsPanel.RenderSimple();
    break;
  default:
    break;
  }
}

} // namespace gits::gui
