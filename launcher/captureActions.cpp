// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureActions.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cwchar>
#include <optional>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "context.h"
#include "fileActions.h"
#include <yaml-cpp/yaml.h>
#include "launcherActions.h"
#include "eventBus.h"
#include "labels.h"

namespace gits::gui::capture_actions {
namespace {

std::optional<uint32_t> FindMatchingProcessPid(const std::filesystem::path& executablePath) {
#ifdef _WIN32
  std::error_code targetPathError;
  const auto normalizedTargetProcessPath =
      std::filesystem::weakly_canonical(executablePath, targetPathError).wstring();
  const auto targetProcessPath =
      targetPathError ? executablePath.wstring() : normalizedTargetProcessPath;

  HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (processSnapshot == INVALID_HANDLE_VALUE) {
    LOG_WARNING << Labels::LOG_CAPTURE_MONITORING_PROCESS_SNAPSHOT_FAILURE;
    return std::nullopt;
  }

  PROCESSENTRY32W processEntry = {};
  processEntry.dwSize = sizeof(PROCESSENTRY32W);

  if (Process32FirstW(processSnapshot, &processEntry)) {
    do {
      HANDLE processHandle =
          OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processEntry.th32ProcessID);
      if (processHandle != nullptr) {
        std::wstring runningProcessPath(32768, L'\0');
        DWORD runningProcessPathLength = static_cast<DWORD>(runningProcessPath.size());
        if (QueryFullProcessImageNameW(processHandle, 0, runningProcessPath.data(),
                                       &runningProcessPathLength) != 0) {
          runningProcessPath.resize(runningProcessPathLength);

          std::error_code runningPathError;
          const auto normalizedRunningProcessPath =
              std::filesystem::weakly_canonical(std::filesystem::path(runningProcessPath),
                                                runningPathError)
                  .wstring();

          const auto& candidatePath =
              runningPathError ? runningProcessPath : normalizedRunningProcessPath;

          if (_wcsicmp(candidatePath.c_str(), targetProcessPath.c_str()) == 0) {
            const auto pid = processEntry.th32ProcessID;
            CloseHandle(processHandle);
            CloseHandle(processSnapshot);
            return pid;
          }
        }
        CloseHandle(processHandle);
      }
    } while (Process32NextW(processSnapshot, &processEntry));
  }

  CloseHandle(processSnapshot);
  return std::nullopt;
#else
  std::error_code targetPathError;
  const auto normalizedTargetProcessPath =
      std::filesystem::weakly_canonical(executablePath, targetPathError);
  const auto targetProcessPath = targetPathError ? executablePath : normalizedTargetProcessPath;

  std::error_code procError;
  std::filesystem::directory_iterator procIterator("/proc", procError);
  if (procError) {
    LOG_WARNING << Labels::LOG_CAPTURE_MONITORING_PROC_ENUMERATION_FAILURE;
    return std::nullopt;
  }

  for (const auto& procEntry : procIterator) {
    const auto pidDirName = procEntry.path().filename().string();
    const bool isPidDirectory =
        !pidDirName.empty() && std::all_of(pidDirName.begin(), pidDirName.end(), [](char c) {
          return std::isdigit(static_cast<unsigned char>(c)) != 0;
        });
    if (!isPidDirectory) {
      continue;
    }

    std::error_code symlinkError;
    auto candidateExePath = std::filesystem::read_symlink(procEntry.path() / "exe", symlinkError);
    if (symlinkError) {
      continue;
    }

    std::error_code candidatePathError;
    const auto normalizedCandidatePath =
        std::filesystem::weakly_canonical(candidateExePath, candidatePathError);
    const auto& candidatePath = candidatePathError ? candidateExePath : normalizedCandidatePath;

    if (candidatePath == targetProcessPath) {
      return static_cast<uint32_t>(std::stoul(pidDirName));
    }
  }

  return std::nullopt;
#endif
}

} // namespace

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
  const auto& captureConfigPath = optCaptureConfigPath.value();
  const auto& captureOutputPath = optCaptureOutputPath.value();

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
                       Api api) {
  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "GITS base path: " << gitsBasePath.string() << " doesn't exist";

    return false;
  }

  auto recorderDirectory = gitsBasePath / filesystem_names::RECORDER_DIRECTORY_NAME;

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

std::optional<std::filesystem::path> GetStreamDirectoryFromLog(const std::string& log) {
  const std::string searchPattern = "Stream will be written to: ";

  size_t pos = log.find(searchPattern);
  if (pos != std::string::npos) {
    // Move to the start of the path
    pos += searchPattern.length();

    // Find the end of the line
    size_t endPos = log.find('\n', pos);
    if (endPos == std::string::npos) {
      endPos = log.length();
    }

    // Extract the path
    std::string pathStr = log.substr(pos, endPos - pos);

    // Remove any trailing whitespace/carriage return
    while (!pathStr.empty() &&
           (pathStr.back() == '\r' || pathStr.back() == '\n' || pathStr.back() == ' ')) {
      pathStr.pop_back();
    }

    return std::filesystem::path(pathStr);
  }

  return std::nullopt;
}

void CaptureStream() {
  auto& context = Context::GetInstance();

  context.GITSLogEditor->SetText("");

  const std::map<Api, std::string> stringForApi = {
      {Api::UNKNOWN, "N/A"},      {Api::DIRECTX, "DX"}, {Api::OPENGL, "GL"},   {Api::VULKAN, "VK"},
      {Api::VULKAN_LEGACY, "VK"}, {Api::OPENCL, "CL"},  {Api::LEVELZERO, "L0"}};

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
  if (!FileActions::CopyFileSafe(
          captureConfigPath,
          executablePath.parent_path() /
              filesystem_names::
                  RECORDER_CONFIG_FILENAME)) { // Recorder needs the hardcoded config name
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

  const auto recorderDirectory = gitsBasePath / filesystem_names::RECORDER_DIRECTORY_NAME;
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

  std::filesystem::path targetDirectory = context.GetPathSafe(Path::CAPTURE_TARGET).parent_path();
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
    std::erase_if(filesToRemove, [](const std::string& filename) {
      return filename != filesystem_names::RECORDER_CONFIG_FILENAME;
    });
  }

  if (!cleanupSelections.CleanRecorderConfig) {
    filesToRemove.erase(std::remove(filesToRemove.begin(), filesToRemove.end(),
                                    filesystem_names::RECORDER_CONFIG_FILENAME),
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

void StartPostExitMonitoring(const std::filesystem::path& executablePath) {
  std::thread([executablePath]() {
    auto& context = Context::GetInstance();
    constexpr auto pollingInterval = std::chrono::milliseconds(500);
    constexpr auto quietPeriod = QUIET_PERIOD;

    LOG_INFO << Labels::LOG_CAPTURE_MONITORING_STARTED_PREFIX << executablePath.filename().string()
             << Labels::LOG_CAPTURE_MONITORING_STARTED_MIDDLE
             << std::chrono::duration_cast<std::chrono::seconds>(quietPeriod).count()
             << Labels::LOG_CAPTURE_MONITORING_STARTED_SUFFIX;

    bool matchingProcessObserved = false;
    std::chrono::steady_clock::time_point noProcessSince;
    context.CaptureInQuietPeriod = true;
    context.CaptureMonitoredPid = 0;
    context.CaptureQuietPeriodStartTick =
        std::chrono::steady_clock::now().time_since_epoch().count();

    while (true) {
      const auto matchingPid = FindMatchingProcessPid(executablePath);
      const bool isRunning = matchingPid.has_value();
      const auto now = std::chrono::steady_clock::now();

      if (isRunning) {
        context.CaptureMonitoredPid = matchingPid.value();
        context.CaptureInQuietPeriod = false;
        context.CaptureQuietPeriodStartTick = 0;
        if (noProcessSince != std::chrono::steady_clock::time_point()) {
          LOG_INFO << Labels::LOG_CAPTURE_MONITORING_DETECTED_AGAIN;
        } else if (!matchingProcessObserved) {
          LOG_INFO << Labels::LOG_CAPTURE_MONITORING_DETECTED_AFTER_EXIT;
        }
        matchingProcessObserved = true;
        noProcessSince = std::chrono::steady_clock::time_point();
      } else {
        context.CaptureMonitoredPid = 0;
        context.CaptureInQuietPeriod = true;
        if (noProcessSince == std::chrono::steady_clock::time_point()) {
          noProcessSince = now;
          context.CaptureQuietPeriodStartTick = now.time_since_epoch().count();
          LOG_INFO << Labels::LOG_CAPTURE_MONITORING_QUIET_PERIOD_STARTED;
        } else if (now - noProcessSince >= quietPeriod) {
          LOG_INFO << Labels::LOG_CAPTURE_MONITORING_FINALIZING;
          ActionEvent event;
          event.EventType = ActionEvent::Type::Capture;
          event.ActionState = ActionEvent::State::Ended;
          EventBus::GetInstance().publish(event);
          return;
        }
      }

      std::this_thread::sleep_for(pollingInterval);
    }
  }).detach();
}
} // namespace gits::gui::capture_actions
