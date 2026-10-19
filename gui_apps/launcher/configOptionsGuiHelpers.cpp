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
#include "guiActions.h"
#include "configMetadataAuto.h"

namespace gits::gui::config_options_gui_helpers {

bool Trace(GitsAction gitsAction, float indentation) {
  bool changed = false;
  auto& context = Context::GetInstance();

  const char* buttonLabelToUse = gitsAction == GitsAction::CAPTURE
                                     ? Labels::BASE_ON_TARGET_PATH_TRACE
                                     : Labels::BASE_ON_STREAMPATH_TRACE;

  changed |= ImGui::Checkbox(Labels::TRACE_EXPORT, &config_options::TraceEnabled(gitsAction));
  ConfigOptionHelpButton(ConfigMetadata::Common::Shared::Trace::enabled);

  ImGui::BeginDisabled(!config_options::TraceEnabled(gitsAction));
  if (indentation > 0.0f) {
    ImGui::Indent(indentation);
  }

  ImGui::Text(Labels::TRACE_PATH);
  ImGui::SameLine();

  float tracePathInputWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, buttonLabelToUse) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET_TRACE) - 16.0f;

  context_helper::PathInput("###TracePath", Path::TRACE, gitsAction, 0, tracePathInputWidth);

  ImGui::SameLine();
  if (ImGui::Button(buttonLabelToUse)) {
    if (GitsAction::CAPTURE == gitsAction) {
      SetTracePathFromTargetExecutable();
    } else {
      SetTracePathFromInputStream();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_TARGET_TRACE)) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::TRACE, gitsAction});
  }

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
