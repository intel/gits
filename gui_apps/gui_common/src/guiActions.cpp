// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "guiActions.h"

#include <optional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"

#include "baseContext.h"
#include "baseEventBus.h"
#include "baseLabels.h"
#include "baseCommon.h"
#include "configOptions.h"
#include "fileActions.h"

namespace {
using namespace gits::gui;

static constexpr const char* PLAYER_TEMPORARY_CONFIG_FILENAME =
    "gits_player_temporary_config_from_launcher.yml"; // For when we serialize the in-memory configuration into a temporary config file

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
#endif

struct TemporaryConfigInfo {
  std::filesystem::path TemporaryConfigPath = "";
  bool OriginalNeedsRestoring = false;
  std::filesystem::path OriginalBackupPath = "";
};

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

TemporaryConfigInfo PrepareTemporaryConfigFile(const gits::Configuration& config,
                                               const std::filesystem::path& directory) {
  auto& context = BaseContext::GetBaseInstance();

  if (directory.empty()) {
    return TemporaryConfigInfo();
  }

  const auto tmpConfigPath = directory / PLAYER_TEMPORARY_CONFIG_FILENAME;

  if (!gits::Configurator::Save(tmpConfigPath, config)) {
    return TemporaryConfigInfo();
  }

  return {tmpConfigPath, false, ""};
}

void RunPlayer(const std::filesystem::path gitsPlayerPath,
               const std::filesystem::path streamPath,
               const gits::Configuration& configuration,
               ActionEvent::Type eventType,
               std::function<void(bool)> onFinished = nullptr,
               std::function<void(const std::string&)> onOutput = nullptr) {
  auto& context = BaseContext::GetBaseInstance();
  const auto tmpConfigInfo =
      PrepareTemporaryConfigFile(configuration, gitsPlayerPath.parent_path());
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't playback the stream, couldn't create a temporary config file";
    if (onFinished) {
      onFinished(false);
    } else {
      EventBus::GetInstance().publish(eventType, ActionEvent::State::Ended,
                                      ActionEvent::Result::RESULT_FAILURE);
    }
    return;
  }
  auto argsWithTmpConfig = std::vector<std::string>();

#ifdef _WIN32
  argsWithTmpConfig.push_back("--config=" +
                              QuoteWindowsPath(tmpConfigInfo.TemporaryConfigPath.string()));
  argsWithTmpConfig.push_back(QuoteWindowsPath(streamPath.string()));
#else
  argsWithTmpConfig.push_back("--config=" + tmpConfigInfo.TemporaryConfigPath.string());
  argsWithTmpConfig.push_back(streamPath.string());
#endif
  context.SetGITSLogText("");
  if (!onOutput) {
    onOutput = [&context](const std::string& log) { context.AddGITSLogText(log); };
  }

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, argsWithTmpConfig, gitsPlayerPath.parent_path(), std::move(onOutput), nullptr,
      nullptr, [eventType, tmpConfigInfo, onFinished = std::move(onFinished)](bool success) {
        std::error_code error;
        std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath, error);
        if (onFinished) {
          onFinished(success);
          return;
        }
        // Check log for errors and publish result
        const std::string logText = BaseContext::GetBaseInstance().GetGITSLogText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(eventType, ActionEvent::State::Ended,
                                        oom || !success ? ActionEvent::Result::RESULT_FAILURE
                                                        : ActionEvent::Result::RESULT_SUCCESS,
                                        oom ? "E_OUTOFMEMORY" : "");
      });
}
} // namespace
namespace gits::gui::gui_actions {
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

bool DetectBasePaths() {
  // the parent folder of the folder in which the executable being run is:
  // Get the parent folder of the folder in which the executable is being run
#ifdef _WIN32
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
#else
  char exePath[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
  std::filesystem::path exeDir =
      std::filesystem::path(std::string(exePath, (count > 0) ? count : 0)).parent_path();
#endif
  auto& context = BaseContext::GetBaseInstance();
  auto basePath = context.GetPathSafe(Path::GITS_BASE);
  auto detected = !basePath.empty() && IsValidGITSBasePath(basePath);
  if (!detected) {
    detected = IsValidGITSBasePath(exeDir);
    if (detected) {
      context.SetPath(exeDir, Path::GITS_BASE);
      LOG_DEBUG << "Detected base path from executable location: " << exeDir;
    }
  }
  if (!detected) {
    const auto buildDir = exeDir.parent_path().parent_path();
    auto possibleDistFoldernames = {"dist",      "install",      "dist_debug",
                                    "distDebug", "dist_release", "distRelease"};
    for (const auto& distFoldername : possibleDistFoldernames) {
      const auto distDir = buildDir / distFoldername;
      detected = IsValidGITSBasePath(distDir);
      if (detected) {
        context.SetPath(distDir, Path::GITS_BASE);
        LOG_DEBUG << "Detected base path from build/dist relative location: " << distDir;
        break;
      }
    }
  }
  if (!detected) {
    LOG_WARNING << "Could not detect GITS base path. Please set it manually.";
    context.SetPath("", Path::GITS_BASE);
  } else {
    const auto playerConfigPath =
        context.GetPathSafe(Path::GITS_BASE) / "Player" / filesystem_names::PLAYER_CONFIG_FILENAME;
    if (!std::filesystem::exists(context.GetPathSafe(Path::CONFIG, GitsAction::PLAYBACK))) {
      context.SetPath(playerConfigPath, Path::CONFIG, GitsAction::PLAYBACK);
    }
    if (!std::filesystem::exists(context.GetPathSafe(Path::CONFIG, GitsAction::SUBCAPTURE))) {
      context.SetPath(playerConfigPath, Path::CONFIG, GitsAction::SUBCAPTURE);
    }
  }

  return detected;
}

void ResetBasePaths() {
  auto& context = BaseContext::GetBaseInstance();
  context.SetPath("", Path::GITS_BASE);
  DetectBasePaths();
  context.SendAllPathsSetEvents();

  LoadConfigFile(context.SelectedAction);
}

void LoadConfigFile(GitsAction gitsAction) {
  auto& baseContext = BaseContext::GetBaseInstance();
  auto& configurationForMode = baseContext.ConfigurationForAction(gitsAction);

  auto filePath = baseContext.GetPathSafe(Path::CONFIG, gitsAction);
  if (filePath.empty()) {
    configurationForMode.BaseGitsConfigurationStr = "// No file specified.";
    return;
  }

  if (!std::filesystem::exists(filePath)) {

    configurationForMode.BaseGitsConfigurationStr =
        "File:\n> " + filePath.string() + "\ndoesn't exist";
    return;
  }

  auto fhandle = std::ifstream(filePath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    YAML::Node yaml;
    try {
      yaml = YAML::Load(str);
    } catch (const YAML::Exception& e) {
      configurationForMode.BaseGitsConfigurationStr =
          "Couldn't load configuration YAML from file:\n>" + filePath.string() + "\nError:\n" +
          e.what();
      return;
    }
    if (gits::Configurator::LoadInto(yaml, &configurationForMode.BaseGitsConfiguration)) {
      std::optional<YAML::Node> baseOverrides = std::nullopt;
      if (yaml["Overrides"]) {
        baseOverrides = YAML::Clone(yaml["Overrides"]);
        configurationForMode.BaseOverrides = baseOverrides.value();
        configurationForMode.ModifiedOverrides = YAML::Clone(yaml["Overrides"]);
      }
      configurationForMode.BaseGitsConfigurationStr =
          GetYamlStringFromConfig(configurationForMode.BaseGitsConfiguration, baseOverrides);
      configurationForMode.ModifiedGitsConfiguration = configurationForMode.BaseGitsConfiguration;
      baseContext.UpdateInMemoryConfig(gitsAction);
    } else {
      configurationForMode.BaseGitsConfigurationStr =
          "Couldn't deserialize configuration from file:\n>" + filePath.string();
    }
  } else {
    configurationForMode.BaseGitsConfigurationStr =
        "// Could not open file:\n> " + filePath.string();
  }

  config_options::SetLauncherDefaults(gitsAction);
  EventBus::GetInstance().publish(ContextEvent::Type::ConfigFileLoaded, gitsAction);
}

void LoadConfigFromMemory(GitsAction gitsAction) {
  auto& baseContext = BaseContext::GetBaseInstance();
  auto& configurationForMode = baseContext.ConfigurationForAction(gitsAction);

  configurationForMode.ModifiedGitsConfigurationStr = configurationForMode.BaseGitsConfigurationStr;
}

std::optional<std::string> ProcessFileDialog(std::string imGuiDialogKey) {
  std::optional<std::string> result = std::nullopt;
  const auto& parentSize = ImGui::GetWindowSize();
  const auto defaultScale = 0.75f;
  const auto minScale = 0.5f;
  const auto maxScale = 1.0f;
  ImGui::SetNextWindowSize(ImVec2(parentSize.x * defaultScale, parentSize.y * defaultScale),
                           ImGuiCond_FirstUseEver);
  if (ImGuiFileDialog::Instance()->Display(
          imGuiDialogKey, 32, ImVec2(parentSize.x * minScale, parentSize.y * minScale),
          ImVec2(parentSize.x * maxScale, parentSize.y * maxScale))) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      result = ImGuiFileDialog::Instance()->GetFilePathName();
    }
    ImGuiFileDialog::Instance()->Close();
  }
  return result;
}

std::optional<PathEvent> FileDialogs() {
  auto& context = BaseContext::GetBaseInstance();
  if (!context.CurrentFileDialogKey.has_value()) {
    return std::nullopt;
  }
  auto key = context.CurrentFileDialogKey.value();
  if (!ProcessFileDialog(key.ImGuiKey())) {
    return std::nullopt;
  }

  auto path = ImGuiFileDialog::Instance()->GetFilePathName();
  if (path.empty()) {
    path = ImGuiFileDialog::Instance()->GetCurrentPath();
  }
  if (path.empty()) {
    return std::nullopt;
  }

  auto event = PathEvent(key.Path, key.GitsAction, path);
  if (key.Path == Path::GITS_BASE) {
    context.SetPath(path, Path::GITS_BASE);
    DetectBasePaths();
    context.SendAllPathsSetEvents();
  } else if (key.Path == Path::GITS_LOG) {
    EventBus::GetInstance().publish(PathEvent(key.Path, key.GitsAction, path));
  } else if (key.Path == Path::CONFIG_EXPORT) {
    EventBus::GetInstance().publish(PathEvent(key.Path, key.GitsAction, path));
  } else {
    context.SetPath(path, key.Path, key.GitsAction);
  }
  return event;
}

bool SetupFileDialogPath(Path path,
                         GitsAction gitsAction,
                         IGFD::FileDialogConfig* dlgConfig,
                         const std::filesystem::path& customPath) {
  auto contextPath = BaseContext::GetBaseInstance().GetPathSafe(path, gitsAction);
  if (!customPath.empty()) {
    contextPath = customPath;
  }
  if (contextPath.empty()) {
    dlgConfig->path = "";
  } else {
    dlgConfig->path = contextPath.string();
  }
  return true;
}

void ShowFileDialog(FileDialogKey key, const std::filesystem::path& path) {
  auto& context = BaseContext::GetBaseInstance();

  context.CurrentFileDialogKey = key;

  std::optional<std::string> ext;
  IGFD::FileDialogConfig dlgConfig;
  dlgConfig.flags = ImGuiFileDialogFlags_Modal;
  bool proceed = false;

  proceed = SetupFileDialogPath(key.Path, key.GitsAction, &dlgConfig, path);
  switch (key.Path) {
  case Path::CAPTURE_TARGET:
#ifdef _WIN32
    ext = ".exe";
#else
    // We need to use a regex here to also catch files without extension
    ext = "((.*))";
#endif
    break;
  case Path::CONFIG:
    ext = ".yml";
    break;
  case Path::INPUT_STREAM:
    ext = ".gits2";
    break;
  case Path::GITS_LOG:
    ext = ".txt";
    break;
  case Path::CONFIG_EXPORT:
    ext = ".yml";
    break;
  default:
    break;
  }

  if (ext.has_value()) {
    dlgConfig.filePathName = dlgConfig.path;
  }

  const auto title = Labels::DialogTitle(key);
  if (proceed) {
    ImGuiFileDialog::Instance()->OpenDialog(
        key.ImGuiKey(), title, ext.has_value() ? ext.value().c_str() : nullptr, dlgConfig);
  }
}

void PlaybackStreamAsync(const std::filesystem::path streamPath,
                         const Configuration& configuration,
                         std::function<void(bool)> onFinished,
                         std::function<void(const std::string&)> onOutput) {
  auto& context = BaseContext::GetBaseInstance();
  const auto& gitsPlayerPath = context.GetGITSPlayerPath();

  if (gitsPlayerPath.empty()) {
    LOG_ERROR << "Playback called with no player set";
    if (onFinished) {
      onFinished(false);
    }
    return;
  }

  LOG_INFO << "Playing back stream: " << streamPath.string();
  RunPlayer(gitsPlayerPath, streamPath, configuration, ActionEvent::Type::Playback,
            std::move(onFinished), std::move(onOutput));
}

void AnalyzeStreamAsync(const std::filesystem::path streamPath,
                        const std::filesystem::path outputPath,
                        Configuration configuration,
                        bool generateScreenshots,
                        bool generateTrace) {
  auto& context = BaseContext::GetBaseInstance();

  configuration.common.shared.screenshots.enabled = generateScreenshots;
  configuration.common.shared.screenshots.format = ImageFormat::JPEG;
  configuration.common.shared.screenshots.frames =
      "1-123456"; // current implementation in the hope it's long enough
  configuration.common.player.outputDir = (outputPath / "screenshots").string();

  // currently not needed, as the screenshots will indicate the frames
  configuration.common.shared.trace.enabled = generateTrace;
  configuration.common.player.outputTracePath = (outputPath / "trace").string();

  const auto& gitsPlayerPath = context.GetGITSPlayerPath();

  if (gitsPlayerPath.empty()) {
    LOG_ERROR << "Anazlye called with no player set";
    return;
  }

  LOG_INFO << "Running stream analysis for stream: " << streamPath.string();
  RunPlayer(gitsPlayerPath, streamPath, configuration, ActionEvent::Type::StudioAnalysisExecuted);
}

std::string CreateEmailBodyWithLog(const std::string& bodyText, const std::string& logText) {
  return bodyText + logText;
}

void SendEmail(const std::string& recipient, const std::string& subject, const std::string& body) {
  auto urlEncode = [](const std::string& str) -> std::string {
    std::string encoded;
    encoded.reserve(str.size() * 3);
    for (unsigned char c : str) {
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
          c == '_' || c == '.' || c == '~') {
        encoded += static_cast<char>(c);
      } else if (c == ' ') {
        encoded += "%20";
      } else if (c == '\n') {
        encoded += "%0A";
      } else if (c == '\r') {
      } else {
        char buf[4];
        snprintf(buf, sizeof(buf), "%%%02X", c);
        encoded += buf;
      }
    }
    return encoded;
  };

  std::string mailto =
      "mailto:" + recipient + "?subject=" + urlEncode(subject) + "&body=" + urlEncode(body);
#ifdef _WIN32
  ShellExecuteA(nullptr, "open", mailto.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
  std::string command = "xdg-open \"" + mailto + "\"";
  if (std::system(command.c_str()) != 0) {
    LOG_WARNING << "Failed to open email client via xdg-open";
  }
#endif
}

void SendLogByEmail(const std::string& recipient,
                    const std::string& subject,
                    const std::string& body) {
  SendEmail(recipient, subject, body);
  LOG_INFO << "Opening email client to send log to " << Labels::EMAIL_LOG_RECIPIENT;
}

void RestoreOriginalConfigIfNeeded(const TemporaryConfigInfo& tmpConfigInfo) {
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't restore original config file. Path to the temporary file is empty";
    return;
  }
  if (!tmpConfigInfo.OriginalNeedsRestoring) {
    std::error_code removeError;
    std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath, removeError);
    return;
  }
  if (tmpConfigInfo.OriginalBackupPath.empty()) {
    LOG_ERROR << "Couldn't restore original config file. Path to the backup file is empty";
    return;
  }
  if (!std::filesystem::exists(tmpConfigInfo.OriginalBackupPath)) {
    LOG_ERROR << "Couldn't restore original config file. Backup file does not exist: "
              << tmpConfigInfo.OriginalBackupPath;
    return;
  }
  try {
    std::filesystem::copy_file(tmpConfigInfo.OriginalBackupPath, tmpConfigInfo.TemporaryConfigPath,
                               std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(tmpConfigInfo.OriginalBackupPath);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Couldn't restore original config file. Error: " << e.what();
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't restore original config file. Error " << e.what();
  } catch (...) {
    LOG_ERROR << "Couldn't restore original config file. Unknown error occurred.";
  }
}

void OpenURL(const std::string& url) {
#ifdef _WIN32
  system(("start " + url).c_str());
#else
  system(("xdg-open " + url).c_str());
#endif
}

bool OpenFolder(const std::filesystem::path& path) {
  std::filesystem::path normalizedPath = path;
  normalizedPath.make_preferred();

  if (normalizedPath.empty()) {
    LOG_ERROR << "Couldn't open directory. No path was provided.";

    return false;
  }

  if (!std::filesystem::exists(normalizedPath)) {
    LOG_ERROR << "Given directory: " << normalizedPath << " doesn't exist.";

    return false;
  }

  if (!std::filesystem::is_directory(normalizedPath)) {
    LOG_ERROR << "Given path: " << normalizedPath << " is not a directory";

    return false;
  }

#ifdef _WIN32
  int result = system(("explorer " + normalizedPath.string()).c_str());
#else
  int result = system(("xdg-open " + normalizedPath.string()).c_str());
#endif

  return result == 0;
}

bool OpenFolder(const std::string& path) {
  return OpenFolder(std::filesystem::path(path));
}

} // namespace gits::gui::gui_actions
