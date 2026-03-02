// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureActions.h"

#include "context.h"
#include "fileActions.h"
#include <yaml-cpp/yaml.h>
#include "launcherActions.h"
#include "eventBus.h"

namespace {
constexpr const char* RECORDER_CONFIG_FILENAME = "gits_config.yml";
}

namespace gits::gui::capture_actions {
bool UpdateConfigDumpPath() {
  auto& context = Context::GetInstance();

  // TODO: Think of a better way of handling modifying the config from different places
  auto optCaptureConfigPath = context.GetPath(Path::CONFIG, Mode::CAPTURE);
  auto optCaptureOutputPath = context.GetPath(Path::OUTPUT_STREAM, Mode::CAPTURE);

  if (!optCaptureConfigPath.has_value()) {
    LOG_ERROR << "Error updating the capture output path. No config file path was specified.";
    return false;
  }
  if (!optCaptureOutputPath.has_value()) {
    LOG_ERROR << "No capture output path was specified";
    return false;
  }
  const auto captureConfigPath = optCaptureConfigPath.value();
  const auto captureOutputPath = optCaptureOutputPath.value();

  if (captureOutputPath.empty()) {
    LOG_ERROR << "No capture output path was specified";

    return false;
  }

  if (captureConfigPath.empty()) {
    LOG_ERROR << "Error updating the capture output path. No config file path was specified.";

    return false;
  }

  if (!std::filesystem::exists(captureConfigPath)) {
    LOG_ERROR
        << "Error updating the capture output path. Specified capture config file doesn't exist";

    return false;
  }

  // Update the config dump directory path to the specified path + the gits special formatting
  return FileActions::UpdateConfigYamlPath(captureConfigPath,
                                           {"Common", "Recorder", "DumpDirectoryPath"},
                                           (captureOutputPath / "%n%_%p%").string(), true);
}

std::string GetRecorderDirectoryNameForApi(Api api) {
  const std::map<Api, std::string> recorderDirectoryForApi{{Api::UNKNOWN, ""},
                                                           {Api::DIRECTX, "FilesToCopyDirectX"},
                                                           {Api::OPENGL, "FilesToCopyOGL"},
                                                           {Api::VULKAN, "FilesToCopyVulkan"},
                                                           {Api::OPENCL, "FilesToCopyOCL"},
                                                           {Api::LEVELZERO, "FilesToCopyL0"}};

  return recorderDirectoryForApi.at(api);
}

bool CopyRecorderFiles(std::filesystem::path gitsBasePath,
                       std::filesystem::path targetDirectory,
                       Api api) {
  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "GITS base path: " << gitsBasePath.string() << " doesn't exist";

    return false;
  }

  auto recorderDirectory = gitsBasePath / "Recorder";

  auto apiDirectory = recorderDirectory / GetRecorderDirectoryNameForApi(api);

  if (!FileActions::Exists(apiDirectory)) {
    LOG_ERROR << "Recorder directory for selected API: " << apiDirectory.string()
              << " doesn't exist";

    return false;
  }

  if (!FileActions::Exists(targetDirectory)) {
    LOG_ERROR << "Target directory: " << targetDirectory << " doesn't exist";

    return false;
  }

  if (!FileActions::CopyDirectoryContents(apiDirectory, targetDirectory)) {
    LOG_ERROR << "Couldn't copy recorder files to the target directory";

    return false;
  }

  return true;
}

std::filesystem::path FindLatestRecorderLog(std::filesystem::path directory) {
  std::filesystem::path latestPath;
  std::filesystem::file_time_type latestLastWriteTime;
  try {
    for (const auto& item : std::filesystem::directory_iterator(directory)) {
      if (item.path().extension() == ".log" &&
          item.path().string().find("gits_") != std::string::npos) {
        if (latestPath.empty() || latestLastWriteTime < item.last_write_time()) {
          latestPath = item.path();
          latestLastWriteTime = item.last_write_time();
        }
      }
    }

    if (latestPath.empty()) {
      LOG_ERROR << "Couldn't find any recorder log";

      return std::filesystem::path();
    }

    LOG_INFO << "Latest recorder log file found: " << latestPath;

    return latestPath;
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Error while trying to find the latest recorder log. Error: " << e.what();
    return std::filesystem::path();
  }
}

void CaptureStream() {
  auto& context = Context::GetInstance();

  context.GITSLogEditor->SetText("");

  const std::map<Api, std::string> stringForApi = {{Api::UNKNOWN, "N/A"}, {Api::DIRECTX, "DX"},
                                                   {Api::OPENGL, "GL"},   {Api::VULKAN, "VK"},
                                                   {Api::OPENCL, "CL"},   {Api::LEVELZERO, "L0"}};

  std::filesystem::path executablePath = context.GetPathSafe(Path::CAPTURE_TARGET);
  if (executablePath.empty()) {
    LOG_ERROR << "No target application was selected for capture";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  std::filesystem::path gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  std::filesystem::path captureConfigPath = context.GetPathSafe(Path::CONFIG, Mode::CAPTURE);
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

  LOG_INFO << "Copying recorder files for capture for API: "
           << stringForApi.at(context.SelectedApiForCapture);
  if (!CopyRecorderFiles(gitsBasePath, executablePath.parent_path(),
                         context.SelectedApiForCapture)) {
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  LOG_INFO << "Copying config file for capture";
  if (!FileActions::CopyFile(captureConfigPath,
                             executablePath.parent_path() /
                                 "gits_config.yml")) { // Recorder needs the hardcoded config name
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return;
  }

  ActionEvent event;
  event.EventType = ActionEvent::Type::Capture;
  event.ActionState = ActionEvent::State::Started;
  EventBus::GetInstance().publish(event);
  FileActions::LaunchExecutableThreadCallbackOnExit(
      executablePath, context.CLIArguments, executablePath.parent_path(),
      [](std::string msg) {
        // We pass an empty lambda to the onOutput argument, since we only care about the recorder log which we load after
        // TODO: Maybe this could change and play nicely with logToConsole
      },
      [executablePath, &context]() {
        ActionEvent event;
        event.EventType = ActionEvent::Type::Capture;
        event.ActionState = ActionEvent::State::Ended;
        EventBus::GetInstance().publish(event);
      });
}

std::vector<std::string> GetRecorderFilesForApi(Api api) {
  auto& context = Context::GetInstance();
  std::filesystem::path gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);

    return std::vector<std::string>();
  }

  const auto recorderDirectory = gitsBasePath / "Recorder";
  const auto apiDirectoryName = GetRecorderDirectoryNameForApi(api);
  if (apiDirectoryName.empty()) {
    return std::vector<std::string>();
  }

  const auto apiDirectory = recorderDirectory / apiDirectoryName;
  std::vector<std::string> filenames;
  try {
    for (const auto& entry : std::filesystem::directory_iterator(apiDirectory)) {
      filenames.push_back(entry.path().filename().string());
    }
  } catch (const std::filesystem::filesystem_error& ex) {
    LOG_ERROR << "Encountered file system error: " << ex.what() << std::endl;
    return std::vector<std::string>();
  }

  return filenames;
}

bool CleanupRecorderFiles(Api api, gui::CapturePanel::CaptureCleanupOptions cleanupSelections) {
  auto& context = Context::GetInstance();
  if (context.SelectedApiForCapture == Api::UNKNOWN) {
    LOG_ERROR << "Couldn't perform cleanup. No capture API was selected.";
    return false;
  }

  std::filesystem::path targetDirectory = context.GetPathSafe(Path::CAPTURE_TARGET);
  if (targetDirectory.empty()) {
    LOG_ERROR << "Couldn't perform cleanup. No target directory was selected.";
    return false;
  }

  if (!std::filesystem::exists(targetDirectory)) {
    LOG_ERROR << "Couldn't perform cleanup. Target directory doesn't exist.";
    return false;
  }

  auto filesToRemove = GetRecorderFilesForApi(context.SelectedApiForCapture);

  if (!cleanupSelections.CleanRecorderFiles) {
    // Recorder files means files other than the config (DLLs etc.)
    std::erase_if(filesToRemove,
                  [](const std::string& filename) { return filename != RECORDER_CONFIG_FILENAME; });
  }

  if (!cleanupSelections.CleanRecorderConfig) {
    filesToRemove.erase(
        std::remove(filesToRemove.begin(), filesToRemove.end(), RECORDER_CONFIG_FILENAME),
        filesToRemove.end());
  }

  if (cleanupSelections.CleanRecorderLog) {
    // Since the recorder log filename will not appear in the initial list, we need to add it if user wants to delete it
    const auto& latestLogPath = FindLatestRecorderLog(targetDirectory);
    if (!latestLogPath.empty() && std::filesystem::exists(latestLogPath)) {
      filesToRemove.push_back(latestLogPath.filename().string());
    }
  }

  // Finally remove all appropriate files and directories from the target directory
  bool result = true;
  for (const auto& filename : filesToRemove) {
    std::filesystem::path fullPath = targetDirectory / filename;
    try {
      if (std::filesystem::exists(fullPath)) {
        LOG_INFO << "Cleaning up: " << fullPath;
        std::filesystem::remove_all(fullPath);
      }
    } catch (const std::filesystem::filesystem_error& ex) {
      LOG_ERROR << "Failed to remove: " << fullPath << " - " << ex.what();
      result = false;
    }
  }

  return result;
}
} // namespace gits::gui::capture_actions
