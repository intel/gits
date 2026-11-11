// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherContext.h"

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
#include "guiActions.h"
#include "launcherLabels.h"
#include "launcherConfig.h"
#include "log.h"
#include "launcherActions.h"
#include "mainWindow.h"
#include "configOptions.h"
#include "baseEventBus.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>

#include "configurationYAMLAuto.h"

#include "configurationYAMLAuto.h"
#include "configurationAuto.h"
#include "configMetadataAuto.h"
#include "yamlUtils.h"

#include "baseContext.h"

namespace gits::gui {

namespace {

#ifdef _WIN32
constexpr const char* REGISTRY_KEYS_FILE_NAME = "registryKeys.yml";
#endif

} // namespace

BaseConfig* LauncherContext::GetBaseConfiguration() {
  return LauncherConfiguration;
}

void LauncherContext::UpdateFixedLauncherArguments() {
  FixedLauncherArguments = "";
  FixedLauncherArguments += TheMainWindow->GetCLIArguments();
}

bool LauncherContext::MainActionAllowed() {
  return false;
}

#ifdef _WIN32
std::filesystem::path LauncherContext::GetRegistryKeysYamlPath() const {
  auto& baseContext = BaseContext::GetBaseInstance();
  const auto gitsBasePath = baseContext.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    return {};
  }

  return gitsBasePath / "Launcher" / REGISTRY_KEYS_FILE_NAME;
}
#endif

bool LauncherContext::IsPlayback() {
  auto& baseContext = BaseContext::GetBaseInstance();
  return baseContext.SelectedAction == GitsAction::PLAYBACK;
}

bool LauncherContext::IsCapture() {
  auto& baseContext = BaseContext::GetBaseInstance();
  return baseContext.SelectedAction == GitsAction::CAPTURE;
}

void LauncherContext::ChangeMode(GitsAction gitsAction) {
  auto& baseContext = BaseContext::GetBaseInstance();
  baseContext.SelectedAction = gitsAction;
  //based on selected action / API
  BtnsSideBar->SetVisible(LauncherContext::SideBarItem::LABEL_STREAM, IsPlayback());
  BtnsSideBar->SetVisible(LauncherContext::SideBarItem::STATS, IsPlayback());
  BtnsSideBar->SetVisible(LauncherContext::SideBarItem::RESOURCE_DUMP,
                          IsPlayback() && baseContext.GetStreamAPI() == Api::DIRECTX);
  // TODO Module entries are unconditionally visible. Gating them the way
  // RESOURCE_DUMP is gated belongs to the module itself, once a module can
  // declare which APIs and modes it supports.

  BtnsSideBar->SelectEntry(LauncherContext::SideBarItem::OPTIONS);

  UpdateFixedLauncherArguments();
  gits::gui::gui_actions::LoadConfigFromMemory(gitsAction);
  baseContext.SetGITSLogText("");

  auto& currentModeConfiguration = baseContext.ConfigurationForAction();

  currentModeConfiguration.ModifiedGitsConfigurationStr =
      GetYamlStringFromConfig(currentModeConfiguration);

  EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ModeChanged);
}

LauncherContext::LauncherContext() : BtnsSideBar(nullptr) {
#ifdef _WIN32
  IsAdminMode = IsElevated();
#endif
}

} // namespace gits::gui
