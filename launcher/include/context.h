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
#include <iostream>
#include <atomic>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "common.h"
#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"
#include "metaDataActions.h"

#include "fileActions.h"
#include "launcherConfig.h"
#include "playbackOptionsPanel.h"
#include "resourceDumpPanel.h"
#include "log.h"
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
    OPTIONS = 0,
    CONFIG,
    RESOURCE_DUMP,
    PLUGINS,
    CLI,
    STATS,
    LOG,
    APP_LOG,
    INFO,
    API_TRACE,
    COUNT
  };

  enum class ConfigSectionItem {
    COMMON = 0,
    DIRECTX,
    OPENGL,
    VULKAN,
    OPENCL,
    LEVELZERO,
    OVERRIDES,
    COUNT
  };

  enum class MetaDataItem {
    CONFIG = 0,
    STATS,
    DIAGS
  };

  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> ConfigEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> LogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> GITSLogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceInfoEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> DiagsEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceConfigEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceStatsEditor;

  std::unique_ptr<gits::gui::PlaybackOptionsPanel> PlaybackOptionsPanel;
  std::unique_ptr<gits::gui::ResourceDumpPanel> ResourceDumpPanel;

  std::shared_ptr<MainWindow> TheMainWindow;

  std::unique_ptr<TextEditorAppender> LogAppender;

  ImGuiHelper::TabGroup<SideBarItem>* BtnsSideBar;
  ImGuiHelper::TabGroup<ConfigSectionItem>* BtnsAPI;
  ImGuiHelper::TabGroup<MetaDataItem>* BtnsMetaData;

  Mode AppMode = Mode::CAPTURE;
  Api SelectedApiForCapture = Api::DIRECTX;
  std::atomic<bool> SubcaptureInProgress = false;

  size_t ImguiIDs = 0;

  std::string FixedLauncherArguments = ""; // Arguments created by the launcher
  std::string CustomArguments = "";        // Custom arguments added by user
  std::vector<std::string> CLIArguments;
  std::string CaptureCustomArguments = "";
  std::vector<std::string> CaptureCLIArguments;
  STREAM_META_DATA MetaData;

  std::map<ConfigSectionItem, int> ConfigSectionLines = {};

  std::optional<FileDialogKeys> CurrentFileDialogKey = std::nullopt;
  LauncherConfig LauncherConfiguration;
  std::optional<float> UIScaleDelta = std::nullopt;

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

  bool IsPlayback();
  bool IsCapture();
  void ChangeMode(Mode mode);
  void UpdatePalette();
  void SendAllPathsSetEvents();
  void SetCaptureAPI(Api api);
};

class TextEditorAppender : public plog::IAppender {
public:
  TextEditorAppender(TextEditorWidget* editor) : m_Editor(editor) {}

  virtual void write(const plog::Record& record) override;

private:
  TextEditorWidget* m_Editor;
};
} // namespace gits::gui
