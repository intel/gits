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

namespace gits::gui::capture_actions {
bool UpdateConfigDumpPath(Context& context) {
  // TODO: Think of a better way of handling modifying the config from different places
  auto captureConfigPath = context.GetPath(gits::gui::Context::Paths::CAPTURE_CONFIG);
  auto captureOutputPath = context.GetPath(gits::gui::Context::Paths::CAPTURE_OUTPUT);

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

bool CopyRecorderFiles(std::filesystem::path gitsBasePath,
                       std::filesystem::path targetDirectory,
                       gui::Context::Api api) {
  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "GITS base path: " << gitsBasePath.string() << " doesn't exist";

    return false;
  }

  auto recorderDirectory = gitsBasePath / "Recorder";

  const std::map<gui::Context::Api, std::string> recorderDirectoryForApi{
      {gui::Context::Api::UNKNOWN, ""},
      {gui::Context::Api::DIRECTX, "FilesToCopyDirectX"},
      {gui::Context::Api::OPENGL, "FilesToCopyOGL"},
      {gui::Context::Api::VULKAN, "FilesToCopyVulkan"},
      {gui::Context::Api::OPENCL, "FilesToCopyOCL"},
      {gui::Context::Api::LEVELZERO, "FilesToCopyL0"}};

  auto apiDirectory = recorderDirectory / recorderDirectoryForApi.at(api);

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

void CaptureStream(gui::Context& context) {
  const auto gitsBasePath = context.GetPath(gui::Context::Paths::GITS_BASE);
  const auto executablePath = context.GetPath(gui::Context::Paths::TARGET);

  context.GITSLogEditor->SetText("");

  const std::map<gui::Context::Api, std::string> stringForApi = {
      {gui::Context::Api::UNKNOWN, "N/A"}, {gui::Context::Api::DIRECTX, "DX"},
      {gui::Context::Api::OPENGL, "GL"},   {gui::Context::Api::VULKAN, "VK"},
      {gui::Context::Api::OPENCL, "CL"},   {gui::Context::Api::LEVELZERO, "L0"}};

  if (context.GetPath(gui::Context::Paths::TARGET).empty()) {
    LOG_ERROR << "No target application was selected for capture";
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "Selected GITS base path for capture: " << gitsBasePath.string()
              << " doesn't exist";
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  if (context.SelectedApiForCapture == gui::Context::Api::UNKNOWN) {
    LOG_ERROR << "No API was selected for capture";
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  LOG_INFO << "Copying recorder files for capture for API: "
           << stringForApi.at(context.SelectedApiForCapture);
  if (!CopyRecorderFiles(context.GetPath(gui::Context::Paths::GITS_BASE),
                         context.GetPath(gui::Context::Paths::TARGET).parent_path(),
                         context.SelectedApiForCapture)) {
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  if (context.GetPath(gui::Context::Paths::CAPTURE_CONFIG).empty()) {
    LOG_ERROR << "No config for capture was selected";
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  LOG_INFO << "Copying config file for capture";
  if (!FileActions::CopyFile(context.GetPath(gui::Context::Paths::CAPTURE_CONFIG),
                             executablePath.parent_path() /
                                 "gits_config.yml")) { // Recorder needs the hardcoded config name
    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

    return;
  }

  context.GITSLogEditor->SetText("");
  context.CaptureInProgress = true;
  FileActions::LaunchExecutableThreadCallbackOnExit(
      executablePath, context.CLIArguments, executablePath.parent_path(),
      [](std::string msg) {
        // We pass an empty lambda to the onOutput argument, since we only care about the recorder log which we load after
        // TODO: Maybe this could change and play nicely with logToConsole
      },
      [executablePath, &context]() {
        LOG_INFO << "Application exit";
        context.CaptureInProgress = false;
        context.RecordingProcessingPending = true;
      });
}
} // namespace gits::gui::capture_actions
