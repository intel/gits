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
#include <cstdlib>
#include <cwchar>
#include <optional>
#include <thread>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "baseContext.h"
#include "fileActions.h"
#include <yaml-cpp/yaml.h>
#include "baseEventBus.h"
#include "baseLabels.h"

namespace gits::gui::capture_actions {
namespace {

std::optional<std::filesystem::path> GetRecorderDirectoryPath(
    const std::filesystem::path& gitsBasePath, Api api) {
  const auto recorderDirectoryName = GetRecorderDirectoryNameForApi(api);
  if (recorderDirectoryName.empty()) {
    LOG_ERROR << "Couldn't determine recorder directory for the selected API";
    return std::nullopt;
  }

  const auto recorderDirectory =
      gitsBasePath / filesystem_names::RECORDER_DIRECTORY_NAME / recorderDirectoryName;
  if (!FileActions::Exists(recorderDirectory)) {
    LOG_ERROR << "Recorder directory for selected API: " << recorderDirectory.string()
              << " doesn't exist";
    return std::nullopt;
  }

  return recorderDirectory;
}

std::optional<uint32_t> FindMatchingProcessPid(const std::filesystem::path& executablePath) {
#ifdef _WIN32
  std::error_code targetPathError;
  auto targetProcessPath =
      std::filesystem::weakly_canonical(executablePath, targetPathError).wstring();
  if (targetPathError) {
    targetProcessPath = executablePath.wstring();
  }

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

std::string GetRecorderDirectoryNameForApi(Api api) {
#ifdef _WIN32
  const std::map<gits::gui::Api, std::string> recorderDirectoryForApi{
      {gits::gui::Api::UNKNOWN, ""},
      {gits::gui::Api::DIRECTX, "FilesToCopyDirectX"},
      {gits::gui::Api::OPENGL, "FilesToCopyOGL"},
      {gits::gui::Api::OPENGL_LEGACY, "FilesToCopyOGLLegacy"},
      {gits::gui::Api::VULKAN, "FilesToCopyVulkan"},
      {gits::gui::Api::VULKAN_LEGACY, "FilesToCopyVulkanLegacy"},
      {gits::gui::Api::OPENCL, "FilesToCopyOCL"},
      {gits::gui::Api::LEVELZERO, "FilesToCopyL0"}};
#else
  const std::map<gits::gui::Api, std::string> recorderDirectoryForApi{
      {gits::gui::Api::UNKNOWN, ""},      {gits::gui::Api::DIRECTX, ""},
      {gits::gui::Api::OPENGL, "OpenGL"}, {gits::gui::Api::OPENGL_LEGACY, "OpenGLLegacy"},
      {gits::gui::Api::VULKAN, "Vulkan"}, {gits::gui::Api::VULKAN_LEGACY, "VulkanLegacy"},
      {gits::gui::Api::OPENCL, "OpenCL"}, {gits::gui::Api::LEVELZERO, "LevelZero"}};
#endif

  return recorderDirectoryForApi.at(api);
}

bool UpdateConfigDumpPath() {
  auto& context = BaseContext::GetBaseInstance();

  // TODO: Think of a better way of handling modifying the config from different places
  auto optCaptureConfigPath = context.GetPath(Path::CONFIG, gits::gui::GitsAction::CAPTURE);
  auto optCaptureOutputPath = context.GetPath(Path::OUTPUT_STREAM, gits::gui::GitsAction::CAPTURE);

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

bool CopyRecorderFiles(const std::filesystem::path& gitsBasePath,
                       const std::filesystem::path& targetDirectory,
                       Api api) {
  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "GITS base path: " << gitsBasePath.string() << " doesn't exist";

    return false;
  }

  const auto apiDirectory = GetRecorderDirectoryPath(gitsBasePath, api);
  if (!apiDirectory.has_value()) {
    return false;
  }

  if (!FileActions::Exists(targetDirectory)) {
    LOG_ERROR << "Target directory: " << targetDirectory << " doesn't exist";

    return false;
  }

  // Copy recorder files, excluding gits_config.yml to avoid overwriting any existing config (launcher will create its own temporary config file)
  std::unordered_set<std::string> excludedFiles{filesystem_names::RECORDER_CONFIG_FILENAME};
  if (!FileActions::CopyDirectoryContentsSafe(apiDirectory.value(), targetDirectory,
                                              excludedFiles)) {
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

namespace {

// Copies the selected capture config file next to the target when
// `configuration` is null, and writes `configuration` there otherwise.
void LaunchCaptureTarget(const Configuration* configuration) {
  auto& context = BaseContext::GetBaseInstance();

  context.SetGITSLogText("");

  const std::map<Api, std::string> stringForApi = {
      {Api::UNKNOWN, "N/A"}, {Api::DIRECTX, "DX"},       {Api::OPENGL, "GL"},
      {Api::VULKAN, "VK"},   {Api::VULKAN_LEGACY, "VK"}, {Api::OPENGL_LEGACY, "GL"},
      {Api::OPENCL, "CL"},   {Api::LEVELZERO, "L0"}};

  std::filesystem::path executablePath = context.GetPathSafe(Path::CAPTURE_TARGET);
  if (executablePath.empty()) {
    LOG_ERROR << "No target application was selected for capture";
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }

  const auto gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }

  std::filesystem::path captureConfigPath =
      context.GetPathSafe(Path::CONFIG, gits::gui::GitsAction::CAPTURE);
  if (configuration == nullptr) {
    if (captureConfigPath.empty()) {
      LOG_ERROR << "No config for capture was selected";
      EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);
      return;
    }
  }

  if (!FileActions::Exists(gitsBasePath)) {
    LOG_ERROR << "Selected GITS base path for capture: " << gitsBasePath.string()
              << " doesn't exist";
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }

  if (context.SelectedApiForCapture == Api::UNKNOWN) {
    LOG_ERROR << "No API was selected for capture";
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }

  LOG_INFO << "Copying recorder files for capture for API: "
           << stringForApi.at(context.SelectedApiForCapture);
  std::unordered_map<std::string, std::string> environmentOverrides;
#ifdef _WIN32
  if (!CopyRecorderFiles(gitsBasePath, executablePath.parent_path(),
                         context.SelectedApiForCapture)) {
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }
#else
  const auto recorderDirectory =
      GetRecorderDirectoryPath(gitsBasePath, context.SelectedApiForCapture);
  if (!recorderDirectory.has_value()) {
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return;
  }

  std::string libraryPath = recorderDirectory.value().string();
  const char* currentLibraryPath = std::getenv("LD_LIBRARY_PATH");
  if (currentLibraryPath != nullptr && currentLibraryPath[0] != '\0') {
    libraryPath += ":" + std::string(currentLibraryPath);
  }
  environmentOverrides.emplace("LD_LIBRARY_PATH", std::move(libraryPath));
  LOG_INFO << "Using recorder directory via LD_LIBRARY_PATH: "
           << recorderDirectory.value().string();
#endif

#ifdef _WIN32
  const auto targetConfigPath =
      executablePath.parent_path() / filesystem_names::RECORDER_CONFIG_FILENAME;
#else
  const auto targetConfigPath =
      recorderDirectory.value() / filesystem_names::RECORDER_CONFIG_FILENAME;
#endif
  if (configuration == nullptr) {
    LOG_INFO << "Copying config file for capture";
    if (!FileActions::CopyFileSafe(captureConfigPath, targetConfigPath)) {
      // Recorder needs the hardcoded config name
      EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

      return;
    }
  } else {
    LOG_INFO << "Writing in-memory config for capture to: " << targetConfigPath;
    if (!Configurator::Save(targetConfigPath, *configuration)) {
      LOG_ERROR << "Failed to write in-memory config for capture to: " << targetConfigPath;
      EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);
      return;
    }
  }

  ActionEvent event;
  event.EventType = ActionEvent::Type::Capture;
  event.ActionState = ActionEvent::State::Started;
  EventBus::GetInstance().publish(event);
  FileActions::LaunchExecutableThreadCallbackOnExit(
      executablePath, context.CLIArguments, executablePath.parent_path(), nullptr,
      [executablePath, &context]() {
        ActionEvent event;
        event.EventType = ActionEvent::Type::Capture;
        event.ActionState = ActionEvent::State::Ended;
        EventBus::GetInstance().publish(event);
      },
      nullptr, nullptr, environmentOverrides);
}

} // namespace

void CaptureStream(bool useInMemoryConfig) {
  if (useInMemoryConfig) {
    CaptureStream(BaseContext::GetBaseInstance()
                      .ConfigurationForAction(GitsAction::CAPTURE)
                      .ModifiedGitsConfiguration);
  } else {
    LaunchCaptureTarget(nullptr);
  }
}

void CaptureStream(const Configuration& configuration) {
  LaunchCaptureTarget(&configuration);
}

std::vector<std::string> GetRecorderFilesForApi(Api api) {
  auto& context = BaseContext::GetBaseInstance();
  std::filesystem::path gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    LOG_ERROR << "No GITS base path for capture was selected";
    EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ShowAppLog);

    return std::vector<std::string>();
  }

  const auto recorderDirectory = GetRecorderDirectoryPath(gitsBasePath, api);
  if (!recorderDirectory.has_value()) {
    return std::vector<std::string>();
  }

  std::vector<std::string> filenames;
  try {
    for (const auto& entry : std::filesystem::directory_iterator(recorderDirectory.value())) {
      filenames.push_back(entry.path().filename().string());
    }
  } catch (const std::filesystem::filesystem_error& ex) {
    LOG_ERROR << "Encountered file system error: " << ex.what() << std::endl;
    return std::vector<std::string>();
  }

  return filenames;
}

bool CleanupRecorderFiles(Api api, gui::CaptureCleanupOptions cleanupSelections) {
  auto& context = BaseContext::GetBaseInstance();
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
    std::filesystem::path logDirectory = targetDirectory;
#ifndef _WIN32
    const auto gitsBasePath = context.GetPathSafe(Path::GITS_BASE);
    const auto recorderDirectoryName =
        GetRecorderDirectoryNameForApi(context.SelectedApiForCapture);
    const auto recorderDirectory =
        gitsBasePath / filesystem_names::RECORDER_DIRECTORY_NAME / recorderDirectoryName;
    if (!gitsBasePath.empty() && !recorderDirectoryName.empty() &&
        std::filesystem::exists(recorderDirectory)) {
      logDirectory = recorderDirectory;
    }
#endif
    const auto& latestLogPath = FindLatestRecorderLog(logDirectory);
    if (!latestLogPath.empty() && std::filesystem::exists(latestLogPath)) {
      filesToRemove.push_back(latestLogPath.string());
    }
  }

  // Finally remove all appropriate files and directories from the target directory
  bool result = true;
  for (const auto& fileOrPath : filesToRemove) {
    std::filesystem::path fullPath = fileOrPath;
    if (!fullPath.is_absolute()) {
      fullPath = targetDirectory / fullPath;
    }
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
    auto& context = BaseContext::GetBaseInstance();
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
