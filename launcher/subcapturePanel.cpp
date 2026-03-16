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
#include "contextHelper.h"

#include "mainWindow.h"

namespace gits::gui {
typedef gits::gui::Context::SideBarItem SideBarItem;

SubcapturePanel::SubcapturePanel() : BasePanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&SubcapturePanel::PathCallback, this, std::placeholders::_1));
}

void SubcapturePanel::Render() {
  auto& context = Context::GetInstance();
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
    UpdateCLICall();
  }
}

const std::string SubcapturePanel::GetCLIArguments() const {
  auto& context = Context::GetInstance();

  std::string args = "";
  if (SubcaptureConfig.Enabled) {
    args += "--DirectX.Features.Subcapture.Enabled ";
    args += "--DirectX.Features.Subcapture.Frames=\"" + SubcaptureConfig.Range() + "\" ";
    args += "--Common.Player.ExitFrame=" + std::to_string(SubcaptureConfig.EndFrame) + " ";
    auto optSubcaptureOutPuthPath =
        Context::GetInstance().GetPath(Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    if (optSubcaptureOutPuthPath.has_value()) {
      auto subcapturePath = optSubcaptureOutPuthPath.value();
      if (!subcapturePath.empty()) {
        // We append the special GITS directory name format to the subcapture directory path
        args += "--Common.Player.SubcapturePath=\"" + (subcapturePath / "%f%_%r%").string() + "\" ";
      }
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
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::SUBCAPTURE_PATH);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::SUBCAPTURE_PATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 20.0f;

  context_helper::PathInput("###SubcapturePath", Path::OUTPUT_STREAM, Mode::SUBCAPTURE, 0,
                            allocatedWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKeys{Path::OUTPUT_STREAM, Mode::SUBCAPTURE});
  }
  ImGui::PopID();
}

void SubcapturePanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != Mode::SUBCAPTURE) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    LoadConfigFile();
  }

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    auto streamPath = Context::GetInstance().GetPathSafe(Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    if (streamPath.empty()) {
      auto parentPath = Context::GetInstance()
                            .GetPathSafe(Path::INPUT_STREAM, Mode::SUBCAPTURE)
                            .parent_path()
                            .parent_path();
      Context::GetInstance().SetPath(parentPath, Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    }
  }

  UpdateCLICall();
}

} // namespace gits::gui
