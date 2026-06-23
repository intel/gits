// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configOptionsGuiHelpers.h"

#include "configOptions.h"
#include "context.h"
#include "contextHelper.h"
#include "imgui.h"
#include "imGuiHelper.h"
#include "labels.h"
#include "launcherActions.h"
#include "configMetadataAuto.h"

namespace gits::gui::config_options_gui_helpers {

bool Trace(Mode mode, float indentation) {
  bool changed = false;
  auto& context = Context::GetInstance();

  const char* buttonLabelToUse =
      mode == Mode::CAPTURE ? Labels::BASE_ON_TARGET_PATH : Labels::BASE_ON_STREAMPATH;

  changed |= ImGui::Checkbox(Labels::TRACE_EXPORT, &config_options::TraceEnabled(mode));
  ConfigOptionHelpButton(ConfigMetadata::DirectX::Features::Trace::enabled);

  ImGui::BeginDisabled(!config_options::TraceEnabled(mode));
  if (indentation > 0.0f) {
    ImGui::Indent(indentation);
  }

  ImGui::Text(Labels::TRACE_PATH);
  ImGui::SameLine();

  float tracePathInputWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, buttonLabelToUse) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 16.0f;

  context_helper::PathInput("###TracePath", Path::TRACE, mode, 0, tracePathInputWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(buttonLabelToUse)) {
    if (Mode::CAPTURE == mode) {
      SetTracePathFromTargetExecutable();
    } else {
      SetTracePathFromInputStream();
    }
  }
  ImGui::PopID();
  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKey{Path::TRACE, mode});
  }
  ImGui::PopID();

  if (indentation > 0.0f) {
    ImGui::Unindent(indentation);
  }
  ImGui::EndDisabled();

  return changed;
}

void ConfigOptionHelpButton(const ConfigFieldMetadata& option) {
  ImGuiHelper::HelpButton(option.ConfigPath, option.ShortDescription, option.Description);
}

} // namespace gits::gui::config_options_gui_helpers
