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
#include "contextHelper.h"
#include "eventBus.h"

#include "mainWindow.h"

namespace {
using namespace gits::gui;

const std::vector<std::pair<const char*, Api>> apiMappings = {
    {Labels::API_NAME_SHORT_DX, Api::DIRECTX},   {Labels::API_NAME_SHORT_GL, Api::OPENGL},
    {Labels::API_NAME_SHORT_VK, Api::VULKAN},    {Labels::API_NAME_SHORT_CL, Api::OPENCL},
    {Labels::API_NAME_SHORT_L0, Api::LEVELZERO},
};

std::vector<const char*> GetApiNames() {
  std::vector<const char*> names;
  for (const auto& mapping : apiMappings) {
    names.push_back(mapping.first);
  }
  return names;
}

std::map<std::string, Api> GetApiMap() {
  std::map<std::string, Api> apiMap;
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
typedef Context::SideBarItem SideBarItem;

CapturePanel::CapturePanel() : BasePanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&CapturePanel::PathCallback, this, std::placeholders::_1));
  apiToolBar = std::make_unique<ImGuiHelper::TabGroup<Api>>(Labels::API_BUTTONS());
  apiToolBar->SelectEntry(Api::DIRECTX);
}

void CapturePanel::Render() {
  RowTargetPath();
  RowCleanup();
  RowConfigPath();
  RowArguments();
  RowOutputPath();
}

void CapturePanel::RowCleanup() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  const auto style = ImGui::GetStyle();

  ImGui::Text(Labels::CLEANUP);

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
    gui::capture_actions::CleanupRecorderFiles(context.SelectedApiForCapture, CleanupOptions);
  }
  ImGuiHelper::AddTooltip(Labels::FORCE_CLEANUP_HINT);
}

const CapturePanel::CaptureCleanupOptions CapturePanel::GetSelectedCleanupOptions() {
  return CleanupOptions;
}

void CapturePanel::RowTargetPath() {
  const auto style = ImGui::GetStyle();
  const std::vector<const char*> apis = GetApiNames();
  const std::map<std::string, Api> apiForString = GetApiMap();

  auto comboWidth = ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 2.0f;

  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::TARGET);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();

  context_helper::PathInput("###InputPath", Path::CAPTURE_TARGET, Mode::CAPTURE, 0, allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::TARGET_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_TARGET, ImVec2(WidthLastButton(), 0))) {
    ShowFileDialog(FileDialogKeys{Path::CAPTURE_TARGET, Mode::CAPTURE});
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_TARGET_HINT);

  ImGui::Text(Labels::API_LABEL);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);
  if (apiToolBar->Render(true)) {
    // Update the selected API in context when user changes selection
    // is the current path based on the BASE path + API?
    // ==> change it.
    // otherwise: leave it as is, user is explicitly selecting an API for capture that might be different than the one in the config file
    context.SetCaptureAPI(apiToolBar->Selected());
  }
}

void CapturePanel::RowArguments() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CaptureCustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall();
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
    context.CustomArguments.clear();
    UpdateCLICall();
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void CapturePanel::RowConfigPath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CAPTURE_CONFIG);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth = remainingWidth - WidthLastButton();

  context_helper::PathInput("###ConfigPathInput", Path::CONFIG, Mode::CAPTURE, 0, allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CAPTURE_CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_CONFIG, ImVec2(WidthLastButton(), 0))) {
    ShowFileDialog(FileDialogKeys{Path::CONFIG, Mode::CAPTURE});
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CAPTURE_CONFIG_HINT);
}

void CapturePanel::RowOutputPath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CAPTURE_OUTPUT_PATH);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth =
      remainingWidth -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_CAPTURE_OUTPUT_PATH) -
      WidthLastButton() - 8;

  context_helper::PathInput("###OutputPathInput", Path::OUTPUT_STREAM, Mode::CAPTURE, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CAPTURE_OUTPUT_PATH_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_OUTPUT_PATH)) {
    ShowFileDialog(FileDialogKeys{Path::OUTPUT_STREAM, Mode::CAPTURE});
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CAPTURE_OUTPUT_PATH_HINT);
#ifdef _WIN32
  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::OPEN_CAPTURE_OUTPUT_PATH, ImVec2(WidthLastButton(), 0))) {
    const auto outputStreamPath = context.GetPathSafe(Path::OUTPUT_STREAM, Mode::CAPTURE);
    OpenFolder(outputStreamPath);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::OPEN_CAPTURE_OUTPUT_PATH_HINT);
#endif
}

void CapturePanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != Mode::CAPTURE) {
    return;
  }

  switch (pathEvent.EventType) {
  case PathEvent::Type::CONFIG:
    LoadConfigFile();
    break;
  case PathEvent::Type::CAPTURE_TARGET:
    UpdateCLICall();
    break;
  case PathEvent::Type::OUTPUT_STREAM:
    capture_actions::UpdateConfigDumpPath();
    LoadConfigFile();
    break;
  default:
    break;
  }
}
} // namespace gits::gui
