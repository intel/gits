// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <optional>
#include <memory>
#include <atomic>
#include <yaml-cpp/node/node.h>
#include <vector>
#include <configurationAuto.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "common.h"
#include "tabGroup.h"
#include "textEditorWidget.h"
#include "metaDataActions.h"

#include "launcherConfig.h"
#include "launcherPaths.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;

class TextEditorAppender;
class MainWindow;

class Context {
public:
  static Context& GetInstance() {
    static Context context;

    return context;
  }

  LauncherPaths Paths;

  enum class SideBarItem {
    LABEL_GITS = 0,
    OPTIONS,
    YAML_CONFIG,
    LABEL_STREAM,
    RESOURCE_DUMP,
    INFO,
    API_TRACE,
    STATS,
    LABEL_LOG,
    LOG,
    APP_LOG,
    COUNT
  };

  struct ModeConfiguration {
    bool ConfigurationDirty = false;
    std::string BaseGitsConfigurationStr; // This preserves the comments
    std::string ModifiedGitsConfigurationStr;
    std::string ConfigDeltaStr;
    Configuration BaseGitsConfiguration;
    Configuration ModifiedGitsConfiguration;
    YAML::Node BaseOverrides;
    YAML::Node ModifiedOverrides;
    STREAM_META_DATA MetaData; // Only relevant for Playback and Subcapture modes

    ModeConfiguration(bool& baseConfigValidityFlag, bool& modifiedConfigValidityFlag)
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

  ModeConfiguration CaptureGitsConfig;
  ModeConfiguration PlaybackGitsConfig;
  ModeConfiguration SubcaptureGitsConfig;

  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> LogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> GITSLogEditor;

  std::shared_ptr<MainWindow> TheMainWindow;

  std::unique_ptr<TextEditorAppender> LogAppender;

  ImGuiHelper::TabGroup<SideBarItem>* BtnsSideBar;

  Mode AppMode = Mode::CAPTURE;
  Api SelectedApiForCapture = Api::DIRECTX;
  std::atomic<bool> SubcaptureInProgress = false;

  size_t ImguiIDs = 0;

  std::string FixedLauncherArguments = ""; // Arguments created by the launcher
  std::string CustomArguments = "";        // Custom arguments added by user
  std::vector<std::string> CLIArguments;
  std::string CaptureCustomArguments = "";
  std::vector<std::string> CaptureCLIArguments;

  std::optional<FileDialogKey> CurrentFileDialogKey = std::nullopt;
  LauncherConfig LauncherConfiguration;
  std::optional<float> UIScaleDelta = std::nullopt;

  bool IsAdminMode = false;

  bool ShouldQuit = false;

  void UpdateFixedLauncherArguments();

  bool MainActionAllowed();

  void GITSLog(const std::string& msg);

  std::optional<std::filesystem::path> GetPath(Path pathType,
                                               std::optional<Mode> appMode = std::nullopt) const;
  std::filesystem::path GetPathSafe(Path pathType,
                                    std::optional<Mode> appMode = std::nullopt) const;
  bool SetPath(std::filesystem::path path,
               Path pathType,
               std::optional<Mode> appMode = std::nullopt);
  std::filesystem::path GetGITSPlayerPath() const;
#ifdef _WIN32
  std::filesystem::path GetRegistryKeysYamlPath() const;
#endif

  bool IsPlayback();
  bool IsCapture();
  void ChangeMode(Mode mode);
  void UpdatePalette();
  void SendAllPathsSetEvents();
  void SetCaptureAPI(Api api);
  Context::ModeConfiguration& ConfigurationForMode(std::optional<Mode> mode = std::nullopt);
  void UpdateInMemoryConfig(std::optional<Mode> mode = std::nullopt);

private:
  Context();

  void UpdateConfigDelta(std::optional<Mode> mode = std::nullopt);
};

class TextEditorAppender : public plog::IAppender {
public:
  TextEditorAppender(TextEditorWidget* editor) : m_Editor(editor) {}

  virtual void write(const plog::Record& record) override;

private:
  TextEditorWidget* m_Editor;
};
} // namespace gits::gui
