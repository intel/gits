// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "liveWorkloadPanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "studioContext.h"
#include "baseContext.h"
//#include "launcherActions.h"
#include "guiActions.h"
#include "fileActions.h"
#include "studioLabels.h"
#include "captureActions.h"
#include "contextHelper.h"
#include "baseEventBus.h"
#include "moduleRegistry.h"

#include "mainWindow.h"

#include "configurationYAMLAuto.h"

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
  auto labels = {Labels::CHOOSE_TARGET, Labels::CLEAR_ARGUMENTS};

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

LiveWorkloadPanel::LiveWorkloadPanel() : BasePanel(), m_APIToolBar(Labels::API_BUTTONS()) {

  auto& context = StudioContext::GetInstance();
  m_APIToolBar.SelectEntry(Api::DIRECTX);
  auto api = m_APIToolBar.Selected();
  context.SetCaptureAPI(api);

  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&LiveWorkloadPanel::PathCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<FileDropEvent>(
      std::bind(&LiveWorkloadPanel::FileDropCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&LiveWorkloadPanel::ContextCallback, this, std::placeholders::_1));
}

void LiveWorkloadPanel::Render() {
  auto& context = StudioContext::GetInstance();

  WidthLeftColumn = context.TheMainWindow->WidthLeftColumn;

  RowTargetPath();
  RowArguments();
  RowAPI();
  RowLaunchAndCleanup();

  DroppedFilePath.reset();
}

void LiveWorkloadPanel::RowTargetPath() {
  const auto style = ImGui::GetStyle();

  auto comboWidth = ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 2.0f;

  auto& context = StudioContext::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::TARGET);

  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();

  context_helper::PathInput("###InputPath", Path::CAPTURE_TARGET, GitsAction::CAPTURE, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::TARGET_INPUT_HINT);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::CAPTURE_TARGET, GitsAction::CAPTURE);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_TARGET, ImVec2(WidthLastButton(), 0))) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::CAPTURE_TARGET, GitsAction::CAPTURE});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_TARGET_HINT);
}

void LiveWorkloadPanel::RowArguments() {
  auto& context = StudioContext::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.LiveWorkloadArguments, 0,
                               remainingWidth)) {
    // nothing
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
    context.LiveWorkloadArguments.clear();
  }
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void LiveWorkloadPanel::RowAPI() {
  auto& context = StudioContext::GetInstance();
  ImGui::Text(Labels::API_LABEL);
  ImGui::SameLine();

  ImGui::SetCursorPosX(WidthLeftColumn);
  if (m_APIToolBar.Render(true)) {
    auto api = m_APIToolBar.Selected();
    context.SetCaptureAPI(api);
  }
}

void LiveWorkloadPanel::RowLaunchAndCleanup() {
  auto& context = StudioContext::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  if (ImGui::Button(Labels::LAUNCH_WORKLOAD)) {
    auto& config = context.ConfigurationForAction(GitsAction::CAPTURE).ModifiedGitsConfiguration;
    config.common.recorder.enabled = false;
    auto launchConfig = config;
    ModuleRegistry::GetInstance().AddActivePlugins(GitsAction::CAPTURE,
                                                   context.SelectedApiForCapture, launchConfig);
    capture_actions::CaptureStream(launchConfig);
  }
  ImGui::SameLine();

  ImGuiHelper::Seperator(true);
  ImGui::Text(Labels::CLEANUP);

  ImGui::SameLine();
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

  // Cleanup options
}

void LiveWorkloadPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != GitsAction::CAPTURE) {
    return;
  }

  switch (pathEvent.EventType) {
  case PathEvent::Type::CONFIG:
    gui_actions::LoadConfigFile(GitsAction::CAPTURE);
    break;
  default:
    break;
  }
}

void LiveWorkloadPanel::FileDropCallback(const Event& e) {
  auto& baseContext = BaseContext::GetBaseInstance();
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  if (baseContext.SelectedAction == GitsAction::CAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

void LiveWorkloadPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = StudioContext::GetInstance();
  auto& configurationForMode = context.ConfigurationForAction(GitsAction::CAPTURE);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}
} // namespace gits::gui
