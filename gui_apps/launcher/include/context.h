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
#include <chrono>
#include <cstdint>
#include <yaml-cpp/node/node.h>
#include <vector>
#include <configurationAuto.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "baseContext.h"
#include "baseCommon.h"
#include "tabGroup.h"
#include "textEditorWidget.h"
#include "metaDataActions.h"

#include "launcherConfig.h"
#include "launcherCommon.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;

class MainWindow;

class Context : public BaseContext {
public:
  static Context& GetInstance() {
    static Context context;
    return context;
  }

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

  std::shared_ptr<MainWindow> TheMainWindow;

  ImGuiHelper::TabGroup<SideBarItem>* BtnsSideBar;

  std::string FixedLauncherArguments = ""; // Arguments created by the launcher
  std::string CustomArguments = "";        // Custom arguments added by user
  std::string CaptureCustomArguments = "";
  std::vector<std::string> CaptureCLIArguments;

  LauncherConfig* LauncherConfiguration = nullptr;

  bool IsAdminMode = false;

  Mode CurrentMode = Mode::SIMPLE;
  std::optional<GitsAction> SimpleModeSelectedAction = std::nullopt;

  BaseConfig* GetBaseConfiguration() override;

  void UpdateFixedLauncherArguments();

  bool MainActionAllowed();

#ifdef _WIN32
  std::filesystem::path GetRegistryKeysYamlPath() const;
#endif

  bool IsPlayback();
  bool IsCapture();
  void ChangeMode(GitsAction gitsAction);

private:
  Context();
};

} // namespace gits::gui
