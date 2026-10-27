// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherActions.h"

#include <cerrno>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "ImGuiFileDialog.h"
#include "yaml-cpp/yaml.h"

#include "fileActions.h"
#include "labels.h"
#include "captureActions.h"
#include "log.h"
#include "configurator.h"
#include "metaDataActions.h"
#include "baseEventBus.h"
#include "configOptions.h"

#include "configurationYAMLAuto.h"
#include <iterator>
#include <yaml-cpp/emitter.h>
#include <configurationAuto.h>

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

std::optional<std::string> ValidateYaml(const std::string& text) {
  try {
    YAML::Node node = YAML::Load(text);
    (void)node; // Workaround for unused variable warning
  } catch (const YAML::Exception& e) {
    return "Error validating YAML: " + std::string(e.what());
  }
  return std::nullopt;
}

std::optional<std::string> ValidateGITSConfig(const std::string& config) {
  return gits::Configurator::Instance().Validate(config);
}

#ifdef _WIN32
std::string QuoteWindowsPath(const std::string& path) {
  if (path.empty()) {
    return "\"\"";
  }
  std::string result = path;
  // If path ends with a backslash, we add a trailing dot
  // This way the ending quote doesn't get escaped, and when GITS Player relaunches itself, the path remains correct
  if (!result.empty() && result.back() == '\\') {
    result += '.';
  }
  return "\"" + result + "\"";
}

bool IsElevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }

  TOKEN_ELEVATION elevation = {};
  DWORD size = sizeof(TOKEN_ELEVATION);
  bool result = false;
  if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
    result = elevation.TokenIsElevated != 0;
  }

  CloseHandle(token);
  return result;
}

void RelaunchAsAdmin() {
  wchar_t path[MAX_PATH] = {};
  if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
    return;
  }

  SHELLEXECUTEINFOW sei = {};
  sei.cbSize = sizeof(sei);
  sei.lpVerb = L"runas";
  sei.lpFile = path;
  sei.nShow = SW_NORMAL;

  if (!ShellExecuteExW(&sei)) {
    DWORD error = GetLastError();
    if (error == ERROR_CANCELLED) {
      LOG_WARNING << "[LauncherActions] User declined UAC prompt\n";
    } else {
      LOG_ERROR << "[LauncherActions] Failed to relaunch as admin: " << error << "\n";
    }
    return;
  }

  LOG_INFO << "[LauncherActions] Relaunched as admin, closing current instance\n";
  PostQuitMessage(0);
}
#endif

void UpdateCLICall() {
  auto& context = Context::GetInstance();
  context.UpdateFixedLauncherArguments();

  switch (context.SelectedAction) {
  case GitsAction::PLAYBACK:
  // [[fallthrough]]
  case GitsAction::SUBCAPTURE: {

    context.CLIArguments.clear();

    context.CLIArguments.push_back(context.CustomArguments);

    const auto& streamPath = context.GetPathSafe(Path::INPUT_STREAM);
#ifdef _WIN32
    context.CLIArguments.push_back(QuoteWindowsPath(streamPath.string()));
#else
    context.CLIArguments.push_back(streamPath.string());
#endif

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    break;
  }
  case GitsAction::CAPTURE: {

    const auto targetExecutable = context.GetPathSafe(Path::CAPTURE_TARGET);

    context.CLIArguments.clear();
    context.CLIArguments.push_back(context.CaptureCustomArguments);

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    break;
  }
  default:
    break;
  }

  EventBus::GetInstance().publish<ContextEvent>(ContextEvent::Type::CLIUpdated);
}

void PlaybackStream() {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

  auto tmpConfigInfo =
      PrepareTemporaryConfigFile(GitsAction::PLAYBACK, gitsPlayerPath.parent_path());
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't playback the stream, couldn't create a temporary config file";
  }
  auto argsWithTmpConfig = context.CLIArguments;

#ifdef _WIN32
  argsWithTmpConfig.push_back("--config=" +
                              QuoteWindowsPath(tmpConfigInfo.TemporaryConfigPath.string()));
#else
  argsWithTmpConfig.push_back("--config=" + tmpConfigInfo.TemporaryConfigPath.string());
#endif

  context.SetGITSLogText("");

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, argsWithTmpConfig, gitsPlayerPath.parent_path(),
      [&context](const std::string& log) { context.AddGITSLogText(log); },
      [&context, tmpConfigInfo = std::move(tmpConfigInfo)]() {
        // Check log for errors and publish result
        const auto& logText = context.GetGITSLogText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(ActionEvent::Type::Playback, ActionEvent::State::Ended,
                                        oom ? ActionEvent::Result::RESULT_FAILURE
                                            : ActionEvent::Result::RESULT_SUCCESS,
                                        oom ? "E_OUTOFMEMORY" : "");
        std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath);
      });
  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void SubcaptureStream() {
  auto& context = Context::GetInstance();
  auto gitsPlayerPath = context.GetGITSPlayerPath();

  auto tmpConfigInfo =
      PrepareTemporaryConfigFile(GitsAction::SUBCAPTURE, gitsPlayerPath.parent_path());
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't playback the stream, couldn't create a temporary config file";
  }
  auto argsWithTmpConfig = context.CLIArguments;
#ifdef _WIN32
  argsWithTmpConfig.push_back("--config=" +
                              QuoteWindowsPath(tmpConfigInfo.TemporaryConfigPath.string()));
#else
  argsWithTmpConfig.push_back("--config=" + tmpConfigInfo.TemporaryConfigPath.string());
#endif

  context.SetGITSLogText("");
  // TODO: Create a generic solution for this
  std::thread([gitsPlayerPath = std::move(gitsPlayerPath), &context,
               tmpConfigInfo = std::move(tmpConfigInfo),
               argsWithTmpConfig = std::move(argsWithTmpConfig)]() {
    context.SubcaptureInProgress = true;
    // since we need to run player twice, consecutively & blocking, we do it in a separate thread
    FileActions::LaunchExecutable(
        gitsPlayerPath, argsWithTmpConfig, true, gitsPlayerPath.parent_path(),
        [&context](const std::string& log) { context.AddGITSLogText(log); });
    FileActions::LaunchExecutable(
        gitsPlayerPath, argsWithTmpConfig, true, gitsPlayerPath.parent_path(),
        [&context](const std::string& log) { context.AddGITSLogText(log); });
    std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath);
    context.SubcaptureInProgress = false;
  }).detach();

  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void GenerateCCode(CCodeExport parameters) {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

  context.SetGITSLogText("");

  //build arguments based on paramenters:
  std::vector<std::string> cliArguments;

  cliArguments.push_back("--DirectX.Player.CCode.Enabled");

  cliArguments.push_back("--DirectX.Player.CCode.CommandsPerBlock=" +
                         std::to_string(parameters.CommandsPerBlock));
  cliArguments.push_back("--DirectX.Player.CCode.WrapApiCalls.Value=" +
                         std::to_string(parameters.WrapAPICalls ? 1 : 0));
  cliArguments.push_back("--DirectX.Player.CCode.CCodePath=" + parameters.CCodePath.string());
  cliArguments.push_back(parameters.StreamPath.string());

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, cliArguments, gitsPlayerPath.parent_path(),
      [&context](const std::string& log) { context.AddGITSLogText(log); },
      [&context]() {
        // Check log for errors and publish result
        const auto& logText = context.GetGITSLogText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(
            ActionEvent::Type::CCodeGeneration, ActionEvent::State::Ended,
            oom ? ActionEvent::Result::RESULT_FAILURE : ActionEvent::Result::RESULT_SUCCESS,
            oom ? "E_OUTOFMEMORY" : "");
      });
  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

TemporaryConfigInfo PrepareTemporaryConfigFile(GitsAction gitsAction,
                                               const std::filesystem::path& directory) {
  auto& context = Context::GetInstance();
  auto& baseContext = BaseContext::GetBaseInstance();

  if (directory.empty()) {
    return TemporaryConfigInfo();
  }

  const auto& configurationForMode = baseContext.ConfigurationForAction(gitsAction);

  if (gitsAction == GitsAction::CAPTURE) {
    auto& dumpPath = config_options::DumpPath(gitsAction);
    if (!dumpPath.string().ends_with("%n%_%p%")) {
      dumpPath = dumpPath / "%n%_%p%";
    }
  }
  if (gitsAction == GitsAction::SUBCAPTURE) {
    auto& subcapturePath = config_options::SubcapturePath(gitsAction);
    if (!subcapturePath.string().ends_with("%f%_%r%")) {
      subcapturePath = subcapturePath / "%f%_%r%";
    }
  }

  auto tmpConfigPath =
      directory /
      (gitsAction == GitsAction::CAPTURE
           ? filesystem_names::RECORDER_CONFIG_FILENAME
           : filesystem_names::
                 PLAYER_TEMPORARY_CONFIG_FILENAME); // For Playback and Subcapture, we can use the --config argument to pass that temporary file so we can have an arbitrary name for it
  auto originalNeedsRestoring = false;
  std::filesystem::path originalBackupPath;
  if (gitsAction == GitsAction::CAPTURE && std::filesystem::exists(tmpConfigPath)) {
    originalNeedsRestoring = true;
    // Generate timestamped backup filename to avoid collisions
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << "gits_config_backup_" << std::put_time(std::localtime(&timeT), "%Y%m%d_%H%M%S") << '_'
        << std::setfill('0') << std::setw(3) << ms.count() << ".yml";
    originalBackupPath = directory / oss.str();
    std::filesystem::copy_file(tmpConfigPath, originalBackupPath,
                               std::filesystem::copy_options::overwrite_existing);
  }

  errno = 0;
  std::ofstream file(tmpConfigPath);
  if (file.is_open()) {
    file << GetYamlStringFromConfig(configurationForMode);
    file.close();
  } else {
    const auto parentDirectory = tmpConfigPath.parent_path();
    std::error_code statusError;
    const auto parentStatus = std::filesystem::status(parentDirectory, statusError);
    LOG_ERROR << "Couldn't open temporary config for writing: " << tmpConfigPath;
    LOG_ERROR << "Parent directory: " << parentDirectory
              << ", exists=" << std::filesystem::exists(parentDirectory)
              << ", status_error=" << (statusError ? statusError.message() : "none")
              << ", permissions=" << static_cast<unsigned>(parentStatus.permissions());
    if (errno != 0) {
      LOG_ERROR << "ofstream open failed with errno=" << errno << " (" << std::strerror(errno)
                << ")";
    }
    return TemporaryConfigInfo();
  }

  return {std::move(tmpConfigPath), originalNeedsRestoring, std::move(originalBackupPath)};
}

std::string GetYamlStringFromConfig(const gits::Configuration& config,
                                    std::optional<YAML::Node> overrides) {
  YAML::Emitter emitter;
  bool result = gits::Configurator::Emit(emitter, config, true, overrides);

  if (!result) {
    LOG_ERROR << "Couldn't get YAML for the current configuration";
    return "";
  }
  return std::string(emitter.c_str());
}

std::string GetYamlStringFromConfig(const BaseContext::GitsActionConfiguration& config) {
  return GetYamlStringFromConfig(config.ModifiedGitsConfiguration, config.ModifiedOverrides);
}

std::string GetRecorderDirectoryNameForApi(Api api) {
#ifdef _WIN32
  switch (api) {
  case Api::DIRECTX:
    return "FilesToCopyDirectX";
  case Api::OPENGL:
    return "FilesToCopyOGL";
  case Api::OPENGL_LEGACY:
    return "FilesToCopyOGLLegacy";
  case Api::VULKAN:
    return "FilesToCopyVulkan";
  case Api::VULKAN_LEGACY:
    return "FilesToCopyVulkanLegacy";
  case Api::OPENCL:
    return "FilesToCopyOCL";
  case Api::LEVELZERO:
    return "FilesToCopyL0";
  default:
    return "";
  }
#else
  switch (api) {
  case Api::OPENGL:
    return "OpenGL";
  case Api::OPENGL_LEGACY:
    return "OpenGLLegacy";
  case Api::VULKAN:
    return "Vulkan";
  case Api::VULKAN_LEGACY:
    return "VulkanLegacy";
  case Api::OPENCL:
    return "OpenCL";
  case Api::LEVELZERO:
    return "LevelZero";
  default:
    return "";
  }
#endif
}

bool IsValidGITSBasePath(const std::filesystem::path& path) {
  if (path.empty()) {
    return false;
  }
#ifdef _WIN32
  const auto playerExecutablePath = path / "Player" / filesystem_names::GITS_PLAYER_WIN;
  const auto recorderDLLPath = path / "Recorder" / filesystem_names::GITS_RECORDER_WIN;
#else
  const auto playerExecutablePath = path / "Player" / filesystem_names::GITS_PLAYER_LINUX;
  const auto recorderDLLPath = path / "Recorder" / filesystem_names::GITS_RECORDER_LINUX;
#endif

  return std::filesystem::exists(playerExecutablePath) && std::filesystem::exists(recorderDLLPath);
}

void SetAllConfigsFromBasePath() {
  auto& context = Context::GetInstance();

  auto captureConfigPath = context.GetRecorderConfigPathForApi(context.SelectedApiForCapture);
  if (std::filesystem::exists(captureConfigPath)) {
    context.SetPath(std::move(captureConfigPath), Path::CONFIG, GitsAction::CAPTURE);
  }
  auto playbackConfigPath = context.GetPlayerConfigPath();
  if (std::filesystem::exists(playbackConfigPath)) {
    context.SetPath(playbackConfigPath, Path::CONFIG, GitsAction::PLAYBACK);
    context.SetPath(std::move(playbackConfigPath), Path::CONFIG, GitsAction::SUBCAPTURE);
  }
}

void NewLauncherSession() {
  auto& context = Context::GetInstance();

  const auto emptyPath = "";

  SetAllConfigsFromBasePath();

  context.SetPath(emptyPath, Path::CAPTURE_TARGET, GitsAction::CAPTURE);
  context.SetPath(emptyPath, Path::OUTPUT_STREAM, GitsAction::CAPTURE);

  context.SetPath(emptyPath, Path::INPUT_STREAM, GitsAction::PLAYBACK);
  context.SetPath(emptyPath, Path::ARTIFACTS, GitsAction::PLAYBACK);
  context.SetPath(emptyPath, Path::TRACE, GitsAction::PLAYBACK);

  context.SetPath(emptyPath, Path::INPUT_STREAM, GitsAction::SUBCAPTURE);
  context.SetPath(emptyPath, Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE);
}

void SetTracePathFromInputStream() {
  auto& context = Context::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK);
  context.SetPath(streamPath.parent_path(), Path::TRACE, GitsAction::PLAYBACK);
}

void SetTracePathFromTargetExecutable() {
  auto& context = Context::GetInstance();
  auto targetPath = context.GetPathSafe(Path::CAPTURE_TARGET, GitsAction::CAPTURE);
  context.SetPath(targetPath.parent_path(), Path::TRACE, GitsAction::CAPTURE);
}

} // namespace gits::gui
