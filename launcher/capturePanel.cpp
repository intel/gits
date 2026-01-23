// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

namespace {
using namespace gits::gui;

const std::vector<std::pair<const char*, Context::Api>> apiMappings = {
    {Labels::NOT_AVAILABLE, Context::Api::UNKNOWN},
    {Labels::API_NAME_SHORT_DX, Context::Api::DIRECTX},
    {Labels::API_NAME_SHORT_GL, Context::Api::OPENGL},
    {Labels::API_NAME_SHORT_VK, Context::Api::VULKAN},
    {Labels::API_NAME_SHORT_CL, Context::Api::OPENCL},
    {Labels::API_NAME_SHORT_L0, Context::Api::LEVELZERO},
};

std::vector<const char*> GetApiNames() {
  std::vector<const char*> names;
  for (const auto& mapping : apiMappings) {
    names.push_back(mapping.first);
  }
  return names;
}

std::map<std::string, Context::Api> GetApiMap() {
  std::map<std::string, Context::Api> apiMap;
  for (const auto& mapping : apiMappings) {
    apiMap[mapping.first] = mapping.second;
  }
  return apiMap;
}

float WidthLastButton() {
  auto labels = {Labels::CHOOSE_TARGET, Labels::CLEAR_ARGUMENTS, Labels::CHOOSE_CAPTURE_CONFIG,
                 Labels::OPEN_CAPTURE_OUTPUT_PATH};

  float result = 0;
  for (const auto& label : labels) {
    auto width = gits::ImGuiHelper::WidthOf(gits::ImGuiHelper::Widgets::Button, label);
    if (width > result) {
      result = width;
    }
  }
  return result;
}
} // namespace

namespace gits::gui {
typedef gits::gui::Context::SideBarItems SideBarItems;

void CapturePanel::Render() {
  RowTargetPath();
  RowCleanup();
  RowConfigPath();
  RowArguments();
  RowOutputPath();
}

void CapturePanel::RowCleanup() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  const auto style = ImGui::GetStyle();

  ImGui::Text(gits::gui::Labels::CLEANUP);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);
  ImGui::Checkbox(Labels::CLEAN_RECORDER_FILES, &CleanupOptions.CleanRecorderFiles);
  ImGuiHelper::AddTooltip(Labels::CLEAN_RECORDER_FILES_HINT);

  ImGui::SameLine();
  ImGui::Checkbox(Labels::CLEAN_RECORDER_CONFIG, &CleanupOptions.CleanRecorderConfig);
  ImGuiHelper::AddTooltip(Labels::CLEAN_RECORDER_CONFIG_HINT);

  ImGui::SameLine();
  ImGui::Checkbox(Labels::CLEAN_RECORDER_LOG, &CleanupOptions.CleanRecorderLog);
  ImGuiHelper::AddTooltip(Labels::CLEAN_RECORDER_LOG_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::FORCE_CLEANUP)) {
    gui::capture_actions::CleanupRecorderFiles(context, context.SelectedApiForCapture,
                                               CleanupOptions);
  }
  ImGuiHelper::AddTooltip(Labels::FORCE_CLEANUP_HINT);
}

const CapturePanel::CaptureCleanupOptions CapturePanel::GetSelectedCleanupOptions() {
  return CleanupOptions;
}

void CapturePanel::RowTargetPath() {
  const auto style = ImGui::GetStyle();
  const std::vector<const char*> apis = GetApiNames();
  const std::map<std::string, gui::Context::Api> apiForString = GetApiMap();

  auto comboWidth = ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 2.0f;

  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(gits::gui::Labels::TARGET);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::API_LABEL) -
                        comboWidth - 4.0f;

  if (ImGuiHelper::InputString("###InputPath", context.TargetPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::TARGET_INPUT_HINT);

  static int selectedItem = 0;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(comboWidth);
  if (ImGui::Combo(Labels::API_LABEL, &selectedItem, apis.data(), apis.size())) {
    auto selectedApi = apis[selectedItem];
    context.SelectedApiForCapture =
        apiForString.count(selectedApi) ? apiForString.at(selectedApi) : gui::Context::Api::UNKNOWN;
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_TARGET, ImVec2(WidthLastButton(), 0))) {
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

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
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
  auto allocatedWidth = remainingWidth - WidthLastButton();

  if (ImGuiHelper::InputString("###ConfigPathInput", context.CaptureConfigPath, 0,
                               allocatedWidth)) {
    UpdateCLICall(context);
    LoadConfigFile(&context);
  }
  ImGuiHelper::AddTooltip(Labels::CAPTURE_CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_CONFIG, ImVec2(WidthLastButton(), 0))) {
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
      WidthLastButton() - 8;

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
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::OPEN_CAPTURE_OUTPUT_PATH, ImVec2(WidthLastButton(), 0))) {
    OpenFolder(context.GetPath(gui::Context::Paths::CAPTURE_OUTPUT));
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::OPEN_CAPTURE_OUTPUT_PATH_HINT);
#endif
}

} // namespace gits::gui
