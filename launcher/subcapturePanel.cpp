// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcapturePanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "context.h"
#include "launcherActions.h"
#include "fileActions.h"
#include "guiController.h"
#include "labels.h"
#include "captureActions.h"

#include "mainWindow.h"

namespace gits::gui {
typedef gits::gui::Context::SideBarItems SideBarItems;

void SubcapturePanel::Render() {
  auto& context = getSharedContext<gui::Context>();
  auto changed = false;
  const float indent = context.TheMainWindow->WidthLeftColumn;
  const float widthLabel = ImGui::CalcTextSize(Labels::SUBCAPTURE_START_FRAME).x * 2.0f + indent;

  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  ImGui::SetNextItemWidth(context.TheMainWindow->WidthLeftColumn);
  ImGui::Text(Labels::SUBCAPTURE);
  ImGui::SameLine();

  ImGui::SetCursorPosX(indent);
  ImGui::SetNextItemWidth(widthLabel / 4.0f);
  changed |= ImGui::InputInt(Labels::SUBCAPTURE_START_FRAME, &SubcaptureConfig.StartFrame, 1, 10);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthLabel / 4.0f);
  changed |= ImGui::InputInt(Labels::SUBCAPTURE_END_FRAME, &SubcaptureConfig.EndFrame, 1, 10);

  ImGui::SetCursorPosX(indent);
  changed |= ImGui::Checkbox(Labels::SUBCAPTURE_OPTIMIZE, &SubcaptureConfig.Optimize);

  ImGui::SetCursorPosX(indent);
  changed |= ImGui::Checkbox(Labels::SUBCAPTURE_EXECUTION_SERIALIZATION,
                             &SubcaptureConfig.ExecutionSerialization);

  RowSubcapturePath();

  if (changed) {
    UpdateCLICall(getSharedContext<Context>());
  }
}

const std::string SubcapturePanel::GetCLIArguments() const {
  auto& context = getSharedContext<gui::Context>();

  std::string args = "";
  if (SubcaptureConfig.Enabled) {
    args += "--DirectX.Features.Subcapture.Enabled ";
    args += "--DirectX.Features.Subcapture.Frames=\"" + SubcaptureConfig.Range() + "\" ";
    if (!context.SubcapturePath.empty()) {
      // We append the special GITS directory name format to the subcapture directory path
      args += "--Common.Player.SubcapturePath=\"" + (context.SubcapturePath / "%f%_%r%").string() +
              "\" ";
    }
    if (!SubcaptureConfig.Optimize) {
      args += "--DirectX.Features.Subcapture.Optimize.Value=0 ";
    }
    if (SubcaptureConfig.ExecutionSerialization) {
      args += "--DirectX.Features.Subcapture.ExecutionSerialization ";
    }
  }
  return args;
}

void SubcapturePanel::RowSubcapturePath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::SUBCAPTURE_PATH);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::SUBCAPTURE_PATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 20.0f;

  if (ImGuiHelper::InputString("###SubcaptureInputPath", context.SubcapturePath, 0,
                               allocatedWidth)) {
    UpdateCLICall(context);
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_SUBCAPTURE_PATH);
  }
  ImGui::PopID();
}

} // namespace gits::gui
