// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePanel.h"

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

void CapturePanel::Render() {
  RowTargetPath();
  RowConfigPath();
  RowArguments();
  RowOutputPath();
}

void CapturePanel::RowTargetPath() {
  const auto style = ImGui::GetStyle();
  const std::vector<const char*> apis = {"N/A", "DX", "GL", "VK", "CL", "L0"};
  const std::map<std::string, gui::Context::Api> apiForString = {
      {"N/A", gui::Context::Api::UNKNOWN}, {"DX", gui::Context::Api::DIRECTX},
      {"GL", gui::Context::Api::OPENGL},   {"VK", gui::Context::Api::VULKAN},
      {"CL", gui::Context::Api::OPENCL},   {"L0", gui::Context::Api::LEVELZERO}};
  auto comboWidth = ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 2.0f;

  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::TARGET);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGui::GetCursorPosX() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITS_BASE_PATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "API") - comboWidth - 4.0f;

  if (ImGuiHelper::InputString("###InputPath", context.TargetPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::TARGET_INPUT_HINT);

  static int selectedItem = 0;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(comboWidth);
  if (ImGui::Combo("API", &selectedItem, apis.data(), apis.size())) {
    auto selectedApi = apis[selectedItem];
    context.SelectedApiForCapture =
        apiForString.count(selectedApi) ? apiForString.at(selectedApi) : gui::Context::Api::UNKNOWN;
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_TARGET_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_TARGET_HINT);
}

void CapturePanel::RowArguments() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CLEAR_ARGUMENTS);
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS)) {
    context.CustomArguments.clear();
    UpdateCLICall(context);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void CapturePanel::RowConfigPath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CAPTURE_CONFIG);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth = remainingWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button,
                                                              Labels::CHOOSE_CAPTURE_CONFIG);

  if (ImGuiHelper::InputString("###ConfigPathInput", context.CaptureConfigPath, 0,
                               allocatedWidth)) {
    UpdateCLICall(context);
    LoadConfigFile(&context);
  }
  ImGuiHelper::AddTooltip(Labels::CAPTURE_CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_CONFIG)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_CAPTURE_CONFIG_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CAPTURE_CONFIG_HINT);
}

void CapturePanel::RowOutputPath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CAPTURE_OUTPUT_PATH);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth =
      remainingWidth -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_CAPTURE_OUTPUT_PATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::OPEN_CAPTURE_OUTPUT_PATH) - 16;

  if (ImGuiHelper::InputString("###OutputPathInput", context.CaptureOutputPath, 0,
                               allocatedWidth)) {
    UpdateCLICall(context);
    LoadConfigFile(&context);
  }
  ImGuiHelper::AddTooltip(Labels::CAPTURE_OUTPUT_PATH_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_OUTPUT_PATH)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_CAPTURE_OUTPUT_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CAPTURE_OUTPUT_PATH_HINT);
#ifdef _WIN32
  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::OPEN_CAPTURE_OUTPUT_PATH)) {
    OpenFolder(context.GetPath(gui::Context::Paths::CAPTURE_OUTPUT));
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::OPEN_CAPTURE_OUTPUT_PATH_HINT);
#endif
}

} // namespace gits::gui
