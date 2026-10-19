// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "baseContext.h"

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <fstream>
#include <utility>
#include <optional>
#include <iostream>
#include <atomic>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"

#include "baseCommon.h"

#include "fileActions.h"
#include "baseConfig.h"
#include "log.h"
#include "baseEventBus.h"
#include "baseLabels.h"
#include "configOptions.h"
#include "configMetadataAuto.h"
#include "guiActions.h"
#include "captureActions.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>
#include "configurationAuto.h"
#include "configurationYAMLAuto.h"
#include "yamlUtils.h"
#include <yaml-cpp/node/node.h>

namespace {
bool IsValidGITSBasePath(const std::filesystem::path& path) {
  if (path.empty()) {
    return false;
  }
#ifdef _WIN32
  const auto playerExecutablePath = path / "Player" / gits::gui::filesystem_names::GITS_PLAYER_WIN;
  const auto recorderDLLPath = path / "Recorder" / gits::gui::filesystem_names::GITS_RECORDER_WIN;
#else
  const auto playerExecutablePath =
      path / "Player" / gits::gui::filesystem_names::GITS_PLAYER_LINUX;
  const auto recorderDLLPath = path / "Recorder" / gits::gui::filesystem_names::GITS_RECORDER_LINUX;
#endif

  return std::filesystem::exists(playerExecutablePath) && std::filesystem::exists(recorderDLLPath);
}
} // namespace

namespace gits::gui {

BaseContext* BaseContext::Instance = nullptr;

BaseContext& BaseContext::GetBaseInstance() {
  if (Instance == nullptr) {
    static BaseContext context;
    (void)context;
  }

  return *Instance;
}

namespace {
// These are here just to satisfy the GITS Configuration constructor interface
// TODO: Could something be changed in regards to initialization of the GITS Configuration struct?
static bool g_BaseConfigValidityFlag = false;
static bool g_ModifiedConfigValidityFlag = false;
} // namespace

BaseContext::BaseContext()
    : CaptureGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag),
      PlaybackGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag),
      SubcaptureGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag) {
  if (Instance == nullptr) {
    Instance = this;
  } else if (Instance != this) {
    throw std::logic_error("Attempted to create more than one BaseContext instance.");
  }

  const auto logConfig =
      TextEditorWidget::Config{.ShowToolbar = true,
                               .ToolBarItems = {TextEditorWidget::TOOL_BAR_ITEMS::EXPORT,
                                                TextEditorWidget::TOOL_BAR_ITEMS::SEND_BY_EMAIL}};

  GITSLogEditor = std::make_unique<TextEditorWidget>("GITSLogEditor");
  GITSLogEditor->SetConfig(logConfig);
  GITSLogEditor->GetEditor().SetReadOnly(true);
  GITSLogEditor->GetEditor().SetShowWhitespaces(false);
  GITSLogEditor->GetEditor().SetTabSize(4);
  const auto gitsLogOnExportCallback = [](std::filesystem::path logPath) {
    auto& baseContext = BaseContext::GetBaseInstance();
    switch (baseContext.SelectedAction) {
    case GitsAction::CAPTURE:
      logPath = baseContext.GetPathSafe(Path::OUTPUT_STREAM, GitsAction::CAPTURE);
      break;
    case GitsAction::PLAYBACK:
      logPath = baseContext.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK);
      break;
    case GitsAction::SUBCAPTURE:
      logPath = baseContext.GetPathSafe(Path::INPUT_STREAM, GitsAction::SUBCAPTURE);
      break;
    }
    if (!logPath.empty()) {
      logPath = logPath.parent_path();
      auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::stringstream ss;
      ss << "gits_log_" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d_%H-%M-%S") << ".txt";
      logPath /= ss.str();
    }
    gui_actions::ShowFileDialog(
        FileDialogKey{.Path = Path::GITS_LOG, .GitsAction = baseContext.SelectedAction}, logPath);
  };
  GITSLogEditor->SetExportCallback(gitsLogOnExportCallback);
  GITSLogEditor->SetSendByEmailCallback([](const std::string& logText) {
    gui_actions::SendLogByEmail(
        Labels::EMAIL_LOG_RECIPIENT, Labels::EMAIL_GITS_LOG_SUBJECT,
        gui_actions::CreateEmailBodyWithLog(Labels::EMAIL_GITS_LOG_BODY_HEADER, logText));
  });

  LogEditor = std::make_unique<TextEditorWidget>("LogEditor");
  LogEditor->SetConfig(logConfig);
  LogEditor->GetEditor().SetReadOnly(true);
  LogEditor->GetEditor().SetShowWhitespaces(false);
  LogEditor->GetEditor().SetTabSize(4);
  const auto launcherLogOnExportCallback = [](std::filesystem::path logPath) {
    auto& baseContext = BaseContext::GetBaseInstance();
    switch (baseContext.SelectedAction) {
    case GitsAction::CAPTURE:
      logPath = baseContext.GetPathSafe(Path::OUTPUT_STREAM, GitsAction::CAPTURE);
      break;
    case GitsAction::PLAYBACK:
      logPath = baseContext.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK);
      break;
    case GitsAction::SUBCAPTURE:
      logPath = baseContext.GetPathSafe(Path::INPUT_STREAM, GitsAction::SUBCAPTURE);
      break;
    }
    if (!logPath.empty()) {
      logPath = logPath.parent_path();
      auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::stringstream ss;
      ss << "launcher_log_" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d_%H-%M-%S")
         << ".txt";
      logPath /= ss.str();
    }
    gui_actions::ShowFileDialog(
        FileDialogKey{.Path = Path::GITS_LOG, .GitsAction = baseContext.SelectedAction}, logPath);
  };
  LogEditor->SetExportCallback(launcherLogOnExportCallback);
  LogEditor->SetSendByEmailCallback([](const std::string& logText) {
    gui_actions::SendLogByEmail(
        Labels::EMAIL_LOG_RECIPIENT, Labels::EMAIL_LAUNCHER_LOG_SUBJECT,
        gui_actions::CreateEmailBodyWithLog(Labels::EMAIL_LAUNCHER_LOG_BODY_HEADER, logText));
  });

  LogAppender = std::make_unique<TextEditorAppender>(LogEditor.get());
  plog::get()->addAppender(LogAppender.get());
}

BaseConfig* BaseContext::GetBaseConfiguration() {
  return nullptr;
}

std::optional<std::filesystem::path> BaseContext::GetPath(
    Path pathType, std::optional<GitsAction> gitsAction) const {
  bool noMode = !gitsAction.has_value();
  GitsAction action = gitsAction.has_value() ? gitsAction.value() : SelectedAction;

  switch (pathType) {

  // universal paths
  case Path::GITS_BASE:
    return Paths.BasePath;

  // unique paths - independent of mode
  case Path::ARTIFACTS:
    if (noMode || action == GitsAction::PLAYBACK) {
      return config_options::OutputDir(GitsAction::PLAYBACK);
    }
    return std::nullopt;
  case Path::CAPTURE_TARGET:
    if (noMode || action == GitsAction::CAPTURE) {
      return Paths.Capture.CaptureTargetPath;
    }
    return std::nullopt;
  case Path::ANALYSIS:
    return Paths.Studio.AnalysisPath;
  default:
    break;
  }

  // paths shared across modes
  switch (action) {
  case GitsAction::PLAYBACK:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Playback.ConfigPath;
    case Path::INPUT_STREAM:
      return Paths.Playback.InputStreamPath;
    case Path::TRACE:
      return config_options::OutputTracePath(GitsAction::PLAYBACK);
    default:
      return std::nullopt;
    }
    break;
  case GitsAction::CAPTURE:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Capture.ConfigPath;
    case Path::OUTPUT_STREAM:
      return config_options::DumpPath(GitsAction::CAPTURE);
    case Path::TRACE:
      return config_options::OutputTracePath(GitsAction::CAPTURE);
    default:
      return std::nullopt;
    }
    break;
  case GitsAction::SUBCAPTURE:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Subcapture.ConfigPath;
    case Path::INPUT_STREAM:
      return Paths.Subcapture.InputStreamPath;
    case Path::OUTPUT_STREAM:
      return config_options::SubcapturePath(GitsAction::SUBCAPTURE);
    default:
      return std::nullopt;
    }
    break;
  }
  return std::nullopt;
}

std::filesystem::path BaseContext::GetPathSafe(Path pathType,
                                               std::optional<GitsAction> gitsAction) const {
  const auto optPath = GetPath(pathType, gitsAction);
  return optPath.has_value() ? optPath.value() : std::filesystem::path();
}

bool BaseContext::SetPath(std::filesystem::path path,
                          Path pathType,
                          std::optional<GitsAction> gitsAction,
                          bool sendEvent) {
  GitsAction action = gitsAction.has_value() ? gitsAction.value() : SelectedAction;
  auto result = false;
  auto configModified = false;
  if (pathType == Path::GITS_BASE) {
    Paths.BasePath = path;
    result = true;
  } else if (pathType == Path::ANALYSIS) {
    Paths.Studio.AnalysisPath = path;
  } else {
    switch (action) {
    case GitsAction::PLAYBACK:
      switch (pathType) {
      case Path::CONFIG:
        Paths.Playback.ConfigPath = path;
        result = true;
        break;
      case Path::INPUT_STREAM:
        Paths.Playback.InputStreamPath = path;
        result = true;
        break;
      case Path::ARTIFACTS:
        config_options::OutputDir(GitsAction::PLAYBACK) = path;
        configModified = true;
        result = true;
        break;
      case Path::TRACE:
        config_options::OutputTracePath(GitsAction::PLAYBACK) = path;
        result = true;
        configModified = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    case GitsAction::CAPTURE:
      switch (pathType) {
      case Path::CONFIG:
        Paths.Capture.ConfigPath = path;
        result = true;
        break;
      case Path::CAPTURE_TARGET:
        Paths.Capture.CaptureTargetPath = path;
        result = true;
        break;
      case Path::OUTPUT_STREAM:
        config_options::DumpPath(GitsAction::CAPTURE) = path;
        configModified = true;
        result = true;
        break;
      case Path::TRACE:
        config_options::OutputTracePath(GitsAction::CAPTURE) = path;
        result = true;
        configModified = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    case GitsAction::SUBCAPTURE:
      switch (pathType) {
      case Path::CONFIG:
        Paths.Subcapture.ConfigPath = path;
        result = true;
        break;
      case Path::INPUT_STREAM:
        Paths.Subcapture.InputStreamPath = path;
        result = true;
        break;
      case Path::OUTPUT_STREAM:
        config_options::SubcapturePath(GitsAction::SUBCAPTURE) = path;
        configModified = true;
        result = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    default:
      result = false;
      break;
    }
  }

  if (result) {
    if (sendEvent) {
      EventBus::GetInstance().publish<PathEvent>(PathEvent(pathType, action));
    }
  }
  if (configModified) {
    ContextEvent event;
    event.EventType = ContextEvent::Type::InMemoryConfigurationChanged;
    EventBus::GetInstance().publish<ContextEvent>(event);
  }

  return result;
}

std::filesystem::path BaseContext::GetGITSPlayerPath() const {
  const auto& basePath = GetPathSafe(Path::GITS_BASE);
  if (basePath.empty()) {
    return std::filesystem::path();
  }
#ifdef _WIN32
  return basePath / "Player" / filesystem_names::GITS_PLAYER_WIN;
#else
  return basePath / "Player" / filesystem_names::GITS_PLAYER_LINUX;
#endif
};

std::filesystem::path BaseContext::GetRecorderConfigPathForApi(Api api) const {
  return Paths.BasePath / filesystem_names::RECORDER_DIRECTORY_NAME /
         capture_actions::GetRecorderDirectoryNameForApi(api) /
         filesystem_names::RECORDER_CONFIG_FILENAME;
}

std::filesystem::path BaseContext::GetPlayerConfigPath() const {
  const auto playerConfigPath =
      Paths.BasePath / "Player" / filesystem_names::PLAYER_CONFIG_FILENAME;
  if (std::filesystem::exists(playerConfigPath)) {
    return playerConfigPath;
  }
  return "";
}

bool BaseContext::DetectBasePaths() {
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
  auto detected = !Paths.BasePath.empty() && IsValidGITSBasePath(Paths.BasePath);
  if (!detected) {
    detected = IsValidGITSBasePath(exeDir);
    if (detected) {
      Paths.BasePath = exeDir;
      LOG_DEBUG << "Detected base path from executable location: " << Paths.BasePath;
    }
  }
  if (!detected) {
    const auto buildDir = exeDir.parent_path().parent_path().parent_path();
    auto possibleDistFoldernames = {"dist",      "install",      "dist_debug",
                                    "distDebug", "dist_release", "distRelease"};
    for (const auto& distFoldername : possibleDistFoldernames) {
      const auto distDir = buildDir / distFoldername;
      detected = IsValidGITSBasePath(distDir);
      if (detected) {
        Paths.BasePath = distDir;
        LOG_DEBUG << "Detected base path from build/dist relative location: " << Paths.BasePath;
        break;
      }
    }
  }
  if (!detected) {
    LOG_WARNING << "Could not detect GITS base path. Please set it manually.";
    Paths.BasePath = "";
  } else {
    const auto playerConfigPath =
        Paths.BasePath / "Player" / filesystem_names::PLAYER_CONFIG_FILENAME;
    if (!std::filesystem::exists(Paths.Playback.ConfigPath)) {
      Paths.Playback.ConfigPath = playerConfigPath;
    }
    if (!std::filesystem::exists(Paths.Subcapture.ConfigPath)) {
      Paths.Subcapture.ConfigPath = playerConfigPath;
    }
  }

  return detected;
}

void BaseContext::ResetBasePaths() {
  Paths.BasePath = "";
  DetectBasePaths();
  SendAllPathsSetEvents();

  gui_actions::LoadConfigFile(SelectedAction);
}

BaseContext::GitsActionConfiguration& BaseContext::ConfigurationForAction(
    std::optional<GitsAction> gitsAction) {
  if (!gitsAction.has_value()) {
    gitsAction = SelectedAction;
  }

  switch (gitsAction.value()) {
  case GitsAction::CAPTURE:
    return CaptureGitsConfig;
  case GitsAction::PLAYBACK:
    return PlaybackGitsConfig;
  case GitsAction::SUBCAPTURE:
    return SubcaptureGitsConfig;
  default:
    // Shouldn't happen
    assert(0 && "Unhandled GITS mode");
    return CaptureGitsConfig;
  }
}

Api BaseContext::GetStreamAPI() {
  auto& config = ConfigurationForAction();
  return config.MetaData.StreamApi;
}

void BaseContext::SetGITSLogText(std::string text) {
  GITSLogEditor->SetText(text);
}

std::string BaseContext::GetGITSLogText() const {
  return GITSLogEditor->GetText();
}

bool BaseContext::HasGITSLogToRender() const {
  return GITSLogEditor->HasTextToRender();
}

void BaseContext::AddGITSLogText(std::string msg) {
  if (msg.empty()) {
    return;
  }
  if (msg == "\n") {
    return;
  }

  size_t offsetFront = msg.front() == '\n' ? 1 : 0;
  size_t offsetBack = msg.back() == '\n' ? 1 : 0;
  if (offsetFront + offsetBack == 0) {
    GITSLogEditor->AppendText(msg, false);
  } else {
    GITSLogEditor->AppendText(msg.substr(offsetFront, msg.size() - offsetFront - offsetBack),
                              false);
  }
}

bool BaseContext::SaveGITSLogToFile(const std::filesystem::path& filePath) const {
  return GITSLogEditor->SaveToFile(filePath);
}

void BaseContext::RenderGITSLogEditor() {
  GITSLogEditor->Render();
}

void BaseContext::RenderLogEditor() {
  LogEditor->Render();
}

void BaseContext::UpdateConfigDelta(std::optional<GitsAction> gitsAction) {
  const auto action = gitsAction.has_value() ? gitsAction.value() : SelectedAction;
  auto& configurationForAction = ConfigurationForAction(action);

  const auto& lookupFunc = [](const std::string& path) -> std::string {
    const auto result = gits::ConfigMetadata::Get(path);
    if (!result) {
      return "";
    }
    return result->ShortDescription;
  };

  try {
    auto node1 =
        YAML::convert<Configuration>::encode(configurationForAction.ModifiedGitsConfiguration);
    if (configurationForAction.ModifiedOverrides) {
      node1[yaml_constants::OVERRIDES_KEY] = configurationForAction.ModifiedOverrides;
    }
    auto node2 = YAML::convert<Configuration>::encode(configurationForAction.BaseGitsConfiguration);
    if (configurationForAction.BaseOverrides) {
      node2[yaml_constants::OVERRIDES_KEY] = configurationForAction.BaseOverrides;
    }

    configurationForAction.ConfigDeltaStr =
        yaml_utils::YamlDeltaGenerator::GenerateDelta(node1, node2, lookupFunc);
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "Error generating config delta: " << e.what();
    configurationForAction.ConfigDeltaStr = "";
  }
}

void BaseContext::UpdateInMemoryConfig(std::optional<GitsAction> gitsAction) {
  const auto action = gitsAction.has_value() ? gitsAction.value() : SelectedAction;

  UpdateConfigDelta(action);
  EventBus::GetInstance().publish<ContextEvent>(
      {ContextEvent::Type::InMemoryConfigurationChanged, action});
}

void BaseContext::UpdatePalette() {
  if (LogEditor) {
    LogEditor->UpdatePalette();
  }
  if (GITSLogEditor) {
    GITSLogEditor->UpdatePalette();
  }
}

void BaseContext::SendAllPathsSetEvents() {
  auto& eventBus = EventBus::GetInstance();
  if (!Paths.BasePath.empty()) {
    eventBus.publish<PathEvent>(PathEvent::Type::GITS_BASE);
  }

  if (!Paths.Capture.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, GitsAction::CAPTURE);
  }
  if (!Paths.Capture.CaptureTargetPath.empty()) {
    eventBus.publish(PathEvent::Type::CAPTURE_TARGET, GitsAction::CAPTURE);
  }
  if (!Paths.Capture.OutputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::OUTPUT_STREAM, GitsAction::CAPTURE);
  }

  if (!Paths.Playback.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, GitsAction::PLAYBACK);
  }
  if (!Paths.Playback.InputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::INPUT_STREAM, GitsAction::PLAYBACK);
  }
  if (!Paths.Playback.ArtifactsPath.empty()) {
    eventBus.publish(PathEvent::Type::ARTIFACTS, GitsAction::PLAYBACK);
  }
  if (!Paths.Playback.TracePath.empty()) {
    eventBus.publish(PathEvent::Type::TRACE, GitsAction::PLAYBACK);
  }

  if (!Paths.Subcapture.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, GitsAction::SUBCAPTURE);
  }
  if (!Paths.Subcapture.InputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::INPUT_STREAM, GitsAction::SUBCAPTURE);
  }
  if (!Paths.Subcapture.OutputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::OUTPUT_STREAM, GitsAction::SUBCAPTURE);
  }
}

const AppPaths& BaseContext::GetAppPaths() const {
  return Paths;
}

void BaseContext::SetAppPaths(const AppPaths& paths) {
  Paths = AppPaths(paths);
}

void BaseContext::SetCaptureAPI(Api api) {
  SelectedApiForCapture = api;
  auto configPath = GetPath(Path::CONFIG, GitsAction::CAPTURE).value();
  auto needsConfigPath = true;
  if (!configPath.empty()) {
    auto basePath = configPath.parent_path().parent_path().parent_path();
    needsConfigPath = basePath == GetPathSafe(Path::GITS_BASE);
  }
  if (needsConfigPath) {
    const auto configPath = GetRecorderConfigPathForApi(api);
    if (std::filesystem::exists(configPath)) {
      SetPath(configPath, Path::CONFIG, GitsAction::CAPTURE);
    }
  }
}

} // namespace gits::gui
