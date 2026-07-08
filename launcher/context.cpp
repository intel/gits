// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "context.h"

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <fstream>
#include <utility>
#include <optional>
#include <iostream>
#include <atomic>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"

#include "captureActions.h"
#include "fileActions.h"
#include "labels.h"
#include "launcherConfig.h"
#include "log.h"
#include "launcherActions.h"
#include "launcherFormatter.h"
#include "mainWindow.h"
#include "eventBus.h"
#include "configOptions.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>

#include "configurationYamlAuto.h"

#include "configurationYamlAuto.h"
#include "configurationAuto.h"
#include "configMetadataAuto.h"
#include "yamlUtils.h"

namespace gits::gui {

namespace {

#ifdef _WIN32
constexpr const char* REGISTRY_KEYS_FILE_NAME = "registryKeys.yml";
#endif

} // namespace

void Context::UpdateFixedLauncherArguments() {
  FixedLauncherArguments = "";
  FixedLauncherArguments += TheMainWindow->GetCLIArguments();
}

bool Context::MainActionAllowed() {
  return false;
}

void Context::GITSLog(const std::string& msg) {
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

std::optional<std::filesystem::path> Context::GetPath(Path pathType,
                                                      std::optional<Mode> appMode) const {
  bool noMode = !appMode.has_value();
  Mode mode = appMode.has_value() ? appMode.value() : AppMode;

  switch (pathType) {

  // universal paths
  case Path::GITS_BASE:
    return Paths.BasePath;

  // unique paths - independent of mode
  case Path::SCREENSHOTS:
    if (noMode || mode == Mode::PLAYBACK) {
      return config_options::OutputDir(Mode::PLAYBACK);
    }
    return std::nullopt;
  case Path::CAPTURE_TARGET:
    if (noMode || mode == Mode::CAPTURE) {
      return Paths.Capture.CaptureTargetPath;
    }
    return std::nullopt;

  default:
    break;
  }

  // paths shared across modes
  switch (mode) {
  case Mode::PLAYBACK:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Playback.ConfigPath;
    case Path::INPUT_STREAM:
      return Paths.Playback.InputStreamPath;
    case Path::TRACE:
      return config_options::OutputTracePath(Mode::PLAYBACK);
    default:
      return std::nullopt;
    }
    break;
  case Mode::CAPTURE:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Capture.ConfigPath;
    case Path::OUTPUT_STREAM:
      return config_options::DumpPath(Mode::CAPTURE);
    case Path::TRACE:
      return config_options::OutputTracePath(Mode::CAPTURE);
    default:
      return std::nullopt;
    }
    break;
  case Mode::SUBCAPTURE:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Subcapture.ConfigPath;
    case Path::INPUT_STREAM:
      return Paths.Subcapture.InputStreamPath;
    case Path::OUTPUT_STREAM:
      return config_options::SubcapturePath(Mode::SUBCAPTURE);
    default:
      return std::nullopt;
    }
    break;
  }
  return std::nullopt;
}

std::filesystem::path Context::GetPathSafe(Path pathType, std::optional<Mode> appMode) const {
  const auto optPath = GetPath(pathType, appMode);
  return optPath.has_value() ? optPath.value() : std::filesystem::path();
}

bool Context::SetPath(std::filesystem::path path, Path pathType, std::optional<Mode> appMode) {
  Mode mode = appMode.has_value() ? appMode.value() : AppMode;
  auto result = false;
  auto configModified = false;
  if (pathType == Path::GITS_BASE) {
    Paths.BasePath = path;
    result = true;
  } else {
    switch (mode) {
    case Mode::PLAYBACK:
      switch (pathType) {
      case Path::CONFIG:
        Paths.Playback.ConfigPath = path;
        result = true;
        break;
      case Path::INPUT_STREAM:
        Paths.Playback.InputStreamPath = path;
        result = true;
        break;
      case Path::SCREENSHOTS:
        config_options::OutputDir(Mode::PLAYBACK) = path;
        configModified = true;
        result = true;
        break;
      case Path::TRACE:
        config_options::OutputTracePath(Mode::PLAYBACK) = path;
        result = true;
        configModified = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    case Mode::CAPTURE:
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
        config_options::DumpPath(Mode::CAPTURE) = path;
        configModified = true;
        result = true;
        break;
      case Path::TRACE:
        config_options::OutputTracePath(Mode::CAPTURE) = path;
        result = true;
        configModified = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    case Mode::SUBCAPTURE:
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
        config_options::SubcapturePath(Mode::SUBCAPTURE) = path;
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
    EventBus::GetInstance().publish<PathEvent>(PathEvent(pathType, mode));
  }
  if (configModified) {
    UpdateInMemoryConfig(mode);
  }

  return result;
}

std::filesystem::path Context::GetGITSPlayerPath() const {
  return Paths.BasePath / "Player" / "gitsPlayer.exe";
}

#ifdef _WIN32
std::filesystem::path Context::GetRegistryKeysYamlPath() const {
  const auto gitsBasePath = GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    return {};
  }

  return gitsBasePath / "Launcher" / REGISTRY_KEYS_FILE_NAME;
}
#endif

bool Context::IsPlayback() {
  return AppMode == Mode::PLAYBACK;
}

bool Context::IsCapture() {
  return AppMode == Mode::CAPTURE;
}

void Context::ChangeMode(Mode mode) {
  AppMode = mode;
  //based on app mode
  BtnsSideBar->SetVisible(Context::SideBarItem::LABEL_STREAM, IsPlayback());
  BtnsSideBar->SetVisible(Context::SideBarItem::STATS, IsPlayback());
  BtnsSideBar->SetVisible(Context::SideBarItem::RESOURCE_DUMP, IsPlayback());

  BtnsSideBar->SelectEntry(Context::SideBarItem::OPTIONS);

  UpdateFixedLauncherArguments();
  gits::gui::LoadConfigFromMemory(mode);
  GITSLogEditor->SetText("");

  auto& currentModeConfiguration = ConfigurationForMode();

  currentModeConfiguration.ModifiedGitsConfigurationStr =
      GetYamlStringFromConfig(currentModeConfiguration);

  EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ModeChanged);
}

void Context::UpdatePalette() {
  if (LogEditor) {
    LogEditor->UpdatePalette();
  }
  if (GITSLogEditor) {
    GITSLogEditor->UpdatePalette();
  }
}

void Context::SendAllPathsSetEvents() {
  auto& eventBus = EventBus::GetInstance();
  if (!Paths.BasePath.empty()) {
    eventBus.publish<PathEvent>(PathEvent::Type::GITS_BASE);
  }

  if (!Paths.Capture.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, Mode::CAPTURE);
  }
  if (!Paths.Capture.CaptureTargetPath.empty()) {
    eventBus.publish(PathEvent::Type::CAPTURE_TARGET, Mode::CAPTURE);
  }
  if (!Paths.Capture.OutputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::OUTPUT_STREAM, Mode::CAPTURE);
  }

  if (!Paths.Playback.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, Mode::PLAYBACK);
  }
  if (!Paths.Playback.InputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::INPUT_STREAM, Mode::PLAYBACK);
  }
  if (!Paths.Playback.ScreenshotsPath.empty()) {
    eventBus.publish(PathEvent::Type::SCREENSHOTS, Mode::PLAYBACK);
  }
  if (!Paths.Playback.TracePath.empty()) {
    eventBus.publish(PathEvent::Type::TRACE, Mode::PLAYBACK);
  }

  if (!Paths.Subcapture.ConfigPath.empty()) {
    eventBus.publish(PathEvent::Type::CONFIG, Mode::SUBCAPTURE);
  }
  if (!Paths.Subcapture.InputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::INPUT_STREAM, Mode::SUBCAPTURE);
  }
  if (!Paths.Subcapture.OutputStreamPath.empty()) {
    eventBus.publish(PathEvent::Type::OUTPUT_STREAM, Mode::SUBCAPTURE);
  }
}

void Context::SetCaptureAPI(Api api) {
  SelectedApiForCapture = api;
  auto needsConfigPath = Paths.Capture.ConfigPath.empty();
  if (!needsConfigPath) {
    auto basePath = Paths.Capture.ConfigPath.parent_path().parent_path().parent_path();
    needsConfigPath = basePath == Paths.BasePath;
  }
  if (needsConfigPath) {
    const auto configPath = GetRecorderConfigPathForApi(api);
    if (std::filesystem::exists(configPath)) {
      SetPath(configPath, Path::CONFIG, Mode::CAPTURE);
    }
  }
}

Context::ModeConfiguration& Context::ConfigurationForMode(std::optional<Mode> mode) {
  if (!mode.has_value()) {
    mode = AppMode;
  }

  switch (mode.value()) {
  case Mode::CAPTURE:
    return CaptureGitsConfig;
  case Mode::PLAYBACK:
    return PlaybackGitsConfig;
  case Mode::SUBCAPTURE:
    return SubcaptureGitsConfig;
  default:
    // Shouldn't happen
    assert(0 && "Unhandled launcher mode");
    return CaptureGitsConfig;
  }
}

void Context::UpdateConfigDelta(std::optional<Mode> mode) {
  const auto appMode = mode.has_value() ? mode.value() : AppMode;
  auto& configurationForMode = ConfigurationForMode(appMode);

  const auto& lookupFunc = [](const std::string& path) -> std::string {
    const auto result = gits::ConfigMetadata::Get(path);
    if (!result) {
      return "";
    }
    return result->ShortDescription;
  };

  try {
    auto node1 =
        YAML::convert<Configuration>::encode(configurationForMode.ModifiedGitsConfiguration);
    if (configurationForMode.ModifiedOverrides) {
      node1[yaml_constants::OVERRIDES_KEY] = configurationForMode.ModifiedOverrides;
    }
    auto node2 = YAML::convert<Configuration>::encode(configurationForMode.BaseGitsConfiguration);
    if (configurationForMode.BaseOverrides) {
      node2[yaml_constants::OVERRIDES_KEY] = configurationForMode.BaseOverrides;
    }

    configurationForMode.ConfigDeltaStr =
        yaml_utils::YamlDeltaGenerator::GenerateDelta(node1, node2, lookupFunc);
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "Error generating config delta: " << e.what();
    configurationForMode.ConfigDeltaStr = "";
  }
}

void Context::UpdateInMemoryConfig(std::optional<Mode> mode) {
  const auto appMode = mode.has_value() ? mode.value() : AppMode;

  UpdateConfigDelta(appMode);
  EventBus::GetInstance().publish<ContextEvent>(
      {ContextEvent::Type::InMemoryConfigurationChanged, appMode});
}

namespace {
// These are here just to satisfy the GITS Configuration constructor interface
// TODO: Could something be changed in regards to initialization of the GITS Configuration struct?
static bool g_BaseConfigValidityFlag = false;
static bool g_ModifiedConfigValidityFlag = false;
} // namespace

Context::Context()
    : CaptureGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag),
      PlaybackGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag),
      SubcaptureGitsConfig(g_BaseConfigValidityFlag, g_ModifiedConfigValidityFlag),
      BtnsSideBar(nullptr) {
#ifdef _WIN32
  IsAdminMode = IsElevated();
#endif
}

void TextEditorAppender::write(const plog::Record& record) {
  if (!m_Editor) {
    return;
  }

  plog::util::nstring str = plog::LauncherFormatter::format(record);
#ifdef _WIN32
  std::string logLine = plog::util::toNarrow(str, 0);
#else
  std::string logLine = str;
#endif
  if (!logLine.empty() && logLine.back() == '\n') {
    logLine.pop_back();
  }
  std::string currentText = m_Editor->GetText();
  currentText += logLine;
  m_Editor->SetText(currentText);
}
} // namespace gits::gui
