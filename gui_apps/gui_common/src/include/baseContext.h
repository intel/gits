// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <fstream>
#include <utility>
#include <optional>
#include <memory>
#include <iostream>
#include <atomic>
#include <string>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "configurationAuto.h"

#include "appPaths.h"
#include "baseCommon.h"
#include "basePanel.h"
#include "metaDataActions.h"
#include "imGuiHelper.h"
#include "textEditorWidget.h"

#include "baseConfig.h"
#include "log.h"

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;
typedef gits::ImGuiHelper::TextEditorAppender TextEditorAppender;

class MainWindow;

class BaseContext {
public:
  static BaseContext& GetBaseInstance();

  struct GitsActionConfiguration {
    bool ConfigurationDirty = false;
    std::string BaseGitsConfigurationStr; // This preserves the comments
    std::string ModifiedGitsConfigurationStr;
    std::string ConfigDeltaStr;
    Configuration BaseGitsConfiguration;
    Configuration ModifiedGitsConfiguration;
    YAML::Node BaseOverrides;
    YAML::Node ModifiedOverrides;
    STREAM_META_DATA MetaData; // Only relevant for Playback and Subcapture modes

    GitsActionConfiguration(bool& baseConfigValidityFlag, bool& modifiedConfigValidityFlag)
        : ConfigurationDirty(false),
          BaseGitsConfigurationStr(""),
          ModifiedGitsConfigurationStr(""),
          ConfigDeltaStr(""),
          BaseGitsConfiguration(baseConfigValidityFlag),
          ModifiedGitsConfiguration(modifiedConfigValidityFlag),
          BaseOverrides(),
          ModifiedOverrides(),
          MetaData() {}
  };

  GitsActionConfiguration CaptureGitsConfig;
  GitsActionConfiguration PlaybackGitsConfig;
  GitsActionConfiguration SubcaptureGitsConfig;

  std::optional<FileDialogKey> CurrentFileDialogKey = std::nullopt;
  std::optional<float> UIScaleDelta = std::nullopt;

  bool ShouldQuit = false;
  virtual BaseConfig* GetBaseConfiguration();

  std::optional<std::filesystem::path> GetPath(
      Path pathType, std::optional<GitsAction> gitsAction = std::nullopt) const;
  std::filesystem::path GetPathSafe(Path pathType,
                                    std::optional<GitsAction> gitsAction = std::nullopt) const;
  bool SetPath(std::filesystem::path path,
               Path pathType,
               std::optional<GitsAction> gitsAction = std::nullopt,
               bool sendEvent = true);

  std::filesystem::path GetGITSPlayerPath() const;
  std::filesystem::path GetRecorderConfigPathForApi(Api api) const;
  std::filesystem::path GetPlayerConfigPath() const;

  bool DetectBasePaths();
  void ResetBasePaths();

  void SetGITSLogText(std::string text);
  std::string GetGITSLogText() const;
  bool HasGITSLogToRender() const;
  void AddGITSLogText(std::string msg);
  bool SaveGITSLogToFile(const std::filesystem::path& filePath) const;

  void RenderGITSLogEditor();
  void RenderLogEditor();

  void UpdatePalette();
  void SendAllPathsSetEvents();

  GitsActionConfiguration& ConfigurationForAction(std::optional<GitsAction> action = std::nullopt);
  void UpdateInMemoryConfig(std::optional<GitsAction> action = std::nullopt);

  GitsAction SelectedAction = GitsAction::PLAYBACK;
#ifdef _WIN32
  Api SelectedApiForCapture = Api::DIRECTX;
#else
  Api SelectedApiForCapture = Api::VULKAN;
#endif
  Api GetStreamAPI();

  void SetCaptureAPI(Api api);

  const AppPaths& GetAppPaths() const;
  void SetAppPaths(const AppPaths& paths);

  std::vector<std::string> CLIArguments;
  std::atomic<bool> SubcaptureInProgress = false;
  std::atomic<uint32_t> CaptureMonitoredPid = 0;
  std::atomic<bool> CaptureInQuietPeriod = false;
  std::atomic<std::chrono::steady_clock::duration::rep> CaptureQuietPeriodStartTick = 0;

protected:
  BaseContext();

private:
  static BaseContext* Instance;

  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> LogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> GITSLogEditor;
  std::unique_ptr<TextEditorAppender> LogAppender;

  void UpdateConfigDelta(std::optional<GitsAction> action = std::nullopt);

  AppPaths Paths;
};
} // namespace gits::gui
