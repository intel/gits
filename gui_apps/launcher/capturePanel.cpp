// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePanel.h"

#include <cstdlib>
#include <unordered_map>

#include "imGuiHelper.h"

#include "resource.h"
#include "context.h"
#include "baseContext.h"
#include "launcherActions.h"
#include "guiActions.h"
#include "fileActions.h"
#include "labels.h"
#include "captureActions.h"
#include "contextHelper.h"
#include "baseEventBus.h"

#include "mainWindow.h"

#include "configurationYAMLAuto.h"

namespace {
using namespace gits::gui;

#ifdef _WIN32
const std::vector<std::pair<const char*, Api>> apiMappings = {
    {Labels::API_NAME_SHORT_DX, Api::DIRECTX},   {Labels::API_NAME_SHORT_GL, Api::OPENGL},
    {Labels::API_NAME_SHORT_VK, Api::VULKAN},    {Labels::API_NAME_SHORT_CL, Api::OPENCL},
    {Labels::API_NAME_SHORT_L0, Api::LEVELZERO},
};
#else
const std::vector<std::pair<const char*, Api>> apiMappings = {
    {Labels::API_NAME_SHORT_GL, Api::OPENGL},
    {Labels::API_NAME_SHORT_VK, Api::VULKAN},
    {Labels::API_NAME_SHORT_CL, Api::OPENCL},
    {Labels::API_NAME_SHORT_L0, Api::LEVELZERO},
};
#endif

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

  EventBus::GetInstance().subscribe<FileDropEvent>(
      std::bind(&CapturePanel::FileDropCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&CapturePanel::ContextCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&CapturePanel::ActionCallback, this, std::placeholders::_1));

  apiToolBar = std::make_unique<ImGuiHelper::TabGroup<Api>>(Labels::API_BUTTONS());
#ifdef _WIN32
  apiToolBar->SelectEntry(Api::DIRECTX);
#else
  apiToolBar->SelectEntry(Api::VULKAN);
#endif
}

void CapturePanel::Render() {
  RowTargetPath();
  RowArguments();
  RowCleanup();
  RowConfigPath();
  RowOutputPath();
  DroppedFilePath.reset();
}

void CapturePanel::RenderSimple() {
  RowTargetPath();
  RowArguments();
  RowOutputPath();
}

void CapturePanel::RowCleanup() {
  auto& context = Context::GetInstance();
  [[maybe_unused]] auto availableWidth = ImGui::GetContentRegionAvail().x;
  [[maybe_unused]] const auto style = ImGui::GetStyle();

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

const CaptureCleanupOptions CapturePanel::GetSelectedCleanupOptions() {
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
    // nothing
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
    context.CaptureCustomArguments.clear();
  }
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

  context_helper::PathInput("###ConfigPathInput", Path::CONFIG, GitsAction::CAPTURE, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CAPTURE_CONFIG_INPUT_HINT);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::CONFIG, GitsAction::CAPTURE);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_CONFIG, ImVec2(WidthLastButton(), 0))) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::CONFIG, GitsAction::CAPTURE});
  }
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

  context_helper::PathInput("###OutputPathInput", Path::OUTPUT_STREAM, GitsAction::CAPTURE, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CAPTURE_OUTPUT_PATH_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_CAPTURE_OUTPUT_PATH)) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::OUTPUT_STREAM, GitsAction::CAPTURE});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CAPTURE_OUTPUT_PATH_HINT);
#ifdef _WIN32
  ImGui::SameLine();
  ImGui::SetNextItemWidth(WidthLastButton());
  if (ImGui::Button(Labels::OPEN_CAPTURE_OUTPUT_PATH, ImVec2(WidthLastButton(), 0))) {
    const auto outputStreamPath = context.GetPathSafe(Path::OUTPUT_STREAM, GitsAction::CAPTURE);
    gui_actions::OpenFolder(outputStreamPath);
  }
  ImGuiHelper::AddTooltip(Labels::OPEN_CAPTURE_OUTPUT_PATH_HINT);
#endif
}

void CapturePanel::CaptureStream() {
  auto& context = Context::GetInstance();

  context.SetGITSLogText("");

  const std::map<Api, std::string> stringForApi = {{Api::UNKNOWN, "N/A"}, {Api::DIRECTX, "DX"},
                                                   {Api::OPENGL, "GL"},   {Api::VULKAN, "VK"},
                                                   {Api::OPENCL, "CL"},   {Api::LEVELZERO, "L0"}};

  std::filesystem::path executablePath = context.GetPathSafe(Path::CAPTURE_TARGET);
  if (executablePath.empty()) {
    LOG_ERROR << "No target application was selected for capture";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  const auto gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  std::filesystem::path captureConfigPath = context.GetPathSafe(Path::CONFIG, GitsAction::CAPTURE);
  if (captureConfigPath.empty()) {
    LOG_ERROR << "No config for capture was selected";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "Selected GITS base path for capture: " << gitsBasePath.string()
              << " doesn't exist";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  if (context.SelectedApiForCapture == Api::UNKNOWN) {
    LOG_ERROR << "No API was selected for capture";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  std::unordered_map<std::string, std::string> environmentOverrides;
#ifdef _WIN32
  LOG_INFO << "Copying recorder files for capture for API: "
           << stringForApi.at(context.SelectedApiForCapture);
  if (!capture_actions::CopyRecorderFiles(gitsBasePath, executablePath.parent_path(),
                                          context.SelectedApiForCapture)) {
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }
#else
  const auto recorderDirectoryName =
      capture_actions::GetRecorderDirectoryNameForApi(context.SelectedApiForCapture);
  const auto recorderDirectory =
      gitsBasePath / filesystem_names::RECORDER_DIRECTORY_NAME / recorderDirectoryName;
  if (recorderDirectoryName.empty() || !FileActions::Exists(recorderDirectory)) {
    LOG_ERROR << "Recorder directory for selected API: " << recorderDirectory.string()
              << " doesn't exist";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  std::string libraryPath = recorderDirectory.string();
  const char* currentLibraryPath = std::getenv("LD_LIBRARY_PATH");
  if (currentLibraryPath != nullptr && currentLibraryPath[0] != '\0') {
    libraryPath += ":" + std::string(currentLibraryPath);
  }
  environmentOverrides.emplace("LD_LIBRARY_PATH", std::move(libraryPath));
  LOG_INFO << "Using recorder directory via LD_LIBRARY_PATH for API: "
           << stringForApi.at(context.SelectedApiForCapture);
#endif

  TmpConfigInfo = PrepareTemporaryConfigFile(GitsAction::CAPTURE,
#ifdef _WIN32
                                             executablePath.parent_path()
#else
                                             recorderDirectory
#endif
  );

  if (TmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't capture the stream. Couldn't create a temporary config file";

    return;
  }

  ActionEvent event;
  event.EventType = ActionEvent::Type::Capture;
  event.ActionState = ActionEvent::State::Started;
  context.CaptureInQuietPeriod = false;
  context.CaptureMonitoredPid = 0;
  context.CaptureQuietPeriodStartTick = 0;
  EventBus::GetInstance().publish(event);

  FileActions::LaunchExecutableThreadCallbackOnExit(
      executablePath, context.CLIArguments, executablePath.parent_path(), nullptr,
      [executablePath]() { capture_actions::StartPostExitMonitoring(executablePath); },
      [&context](uint32_t pid) {
        context.CaptureMonitoredPid = pid;
        context.CaptureInQuietPeriod = false;
        context.CaptureQuietPeriodStartTick = 0;
      },
      nullptr, environmentOverrides);
}

void CapturePanel::PathCallback(const Event& e) {
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

void CapturePanel::FileDropCallback(const Event& e) {
  auto& baseContext = BaseContext::GetBaseInstance();
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  if (baseContext.SelectedAction == GitsAction::CAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

void CapturePanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = Context::GetInstance();
  auto& configurationForMode = context.ConfigurationForAction(GitsAction::CAPTURE);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}

void CapturePanel::ActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);

  if (actionEvent.EventType == ActionEvent::Type::Capture &&
      actionEvent.ActionState == ActionEvent::State::Ended) {
    gui_actions::RestoreOriginalConfigIfNeeded(TmpConfigInfo);
    TmpConfigInfo = TemporaryConfigInfo();
  }
}
} // namespace gits::gui
