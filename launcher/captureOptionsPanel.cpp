// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "CaptureOptionsPanel.h"

#include "imgui.h"

#include "common.h"
#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "eventBus.h"
#include "configOptions.h"
#include "configOptionsGuiHelpers.h"
#include "configMetadataAuto.h"

namespace gits::gui {

CaptureOptionsPanel::CaptureOptionsPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&CaptureOptionsPanel::PathCallback, this, std::placeholders::_1));
};

void CaptureOptionsPanel::Render() {
  auto changed = false;

  changed |=
      ImGui::Checkbox(Labels::RECORDING_ENABLED, &config_options::RecorderEnabled(Mode::CAPTURE));
  config_options_gui_helpers::ConfigOptionHelpButton(ConfigMetadata::Common::Recorder::enabled);

  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  auto& hudApis = config_options::HudEnabled(Mode::CAPTURE);
  auto dxHudEnabled = std::find(hudApis.begin(), hudApis.end(), gits::ApiBool::DX) != hudApis.end();
  changed |= ImGui::Checkbox(Labels::HUD_ENABLED, &dxHudEnabled);
  if (changed) {
    auto it = std::find(hudApis.begin(), hudApis.end(), gits::ApiBool::DX);
    if (dxHudEnabled && it == hudApis.end()) {
      hudApis.push_back(gits::ApiBool::DX);
    } else if (!dxHudEnabled && it != hudApis.end()) {
      hudApis.erase(it);
    }
  }
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Shared::HUD::GroupMetadata);

  changed |= ImGui::Checkbox(Labels::SHADOW_MEMORY, &config_options::ShadowMemory(Mode::CAPTURE));
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Recorder::shadowMemory);

  const auto indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

  changed |= config_options_gui_helpers::Trace(Mode::CAPTURE, indent);

  if (changed) {
    Context::GetInstance().UpdateInMemoryConfig(Mode::CAPTURE);
  }
}

void CaptureOptionsPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);
  if (pathEvent.EventType == PathEvent::Type::CAPTURE_TARGET) {
    auto tracePath = Context::GetInstance().GetPathSafe(Path::TRACE, Mode::CAPTURE);
    if (tracePath.empty()) {
      SetTracePathFromTargetExecutable();
    }
  }
}

} // namespace gits::gui
