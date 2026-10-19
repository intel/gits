// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureOptionsPanel.h"

#include "imgui.h"

#include "baseContext.h"
#include "baseCommon.h"
#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "baseLabels.h"
#include "baseEventBus.h"
#include "configOptions.h"
#include "configOptionsGuiHelpers.h"
#include "configMetadataAuto.h"

namespace gits::gui {

CaptureOptionsPanel::CaptureOptionsPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&CaptureOptionsPanel::PathCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&CaptureOptionsPanel::ContextCallback, this, std::placeholders::_1));
};

void CaptureOptionsPanel::Render() {
  auto changed = false;

  changed |= ImGui::Checkbox(Labels::RECORDING_ENABLED,
                             &config_options::RecorderEnabled(GitsAction::CAPTURE));
  config_options_gui_helpers::ConfigOptionHelpButton(ConfigMetadata::Common::Recorder::enabled);

  auto& context = Context::GetInstance();
  auto api = context.SelectedApiForCapture;

  if (api == Api::DIRECTX || api == Api::VULKAN) {
    const auto hudApi = api == Api::DIRECTX ? gits::ApiBool::DX : gits::ApiBool::VK;
    auto& hudApis = config_options::HudEnabled(GitsAction::CAPTURE);
    auto hudEnabled = std::find(hudApis.begin(), hudApis.end(), hudApi) != hudApis.end();
    if (ImGui::Checkbox(Labels::HUD_ENABLED, &hudEnabled)) {
      changed = true;
      auto it = std::find(hudApis.begin(), hudApis.end(), hudApi);
      if (hudEnabled && it == hudApis.end()) {
        hudApis.push_back(hudApi);
      } else if (!hudEnabled && it != hudApis.end()) {
        hudApis.erase(it);
      }
    }
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Shared::HUD::GroupMetadata);

    if (api == Api::DIRECTX) {
      changed |= ImGui::Checkbox(Labels::SHADOW_MEMORY,
                                 &config_options::ShadowMemory(GitsAction::CAPTURE));
      config_options_gui_helpers::ConfigOptionHelpButton(
          ConfigMetadata::DirectX::Recorder::shadowMemory);
    }

    const auto indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

    changed |= config_options_gui_helpers::Trace(GitsAction::CAPTURE, indent);
  }

  if (changed) {
    BaseContext::GetBaseInstance().UpdateInMemoryConfig(GitsAction::CAPTURE);
  }
}

void CaptureOptionsPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);
  if (pathEvent.EventType == PathEvent::Type::CAPTURE_TARGET) {
    auto tracePath = BaseContext::GetBaseInstance().GetPathSafe(Path::TRACE, GitsAction::CAPTURE);
    if (tracePath.empty()) {
      SetTracePathFromTargetExecutable();
    }
  }
}

void CaptureOptionsPanel::RenderSimple() {
  // Nothing at the moment
}

void CaptureOptionsPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);
  if (contextEvent.Mode == GitsAction::CAPTURE &&
      contextEvent.EventType == ContextEvent::Type::ConfigFileLoaded) {
    auto tracePath = BaseContext::GetBaseInstance().GetPathSafe(Path::TRACE, GitsAction::CAPTURE);
    if (tracePath.empty()) {
      SetTracePathFromTargetExecutable();
    }
  }
}

} // namespace gits::gui
