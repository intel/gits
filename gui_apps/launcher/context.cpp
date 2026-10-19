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
#include "guiActions.h"
#include "labels.h"
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

BaseConfig* Context::GetBaseConfiguration() {
  return LauncherConfiguration;
}

void Context::UpdateFixedLauncherArguments() {
  FixedLauncherArguments = "";
  FixedLauncherArguments += TheMainWindow->GetCLIArguments();
}

bool Context::MainActionAllowed() {
  return false;
}

#ifdef _WIN32
std::filesystem::path Context::GetRegistryKeysYamlPath() const {
  auto& baseContext = BaseContext::GetBaseInstance();
  const auto gitsBasePath = baseContext.GetPathSafe(Path::GITS_BASE);
  if (gitsBasePath.empty()) {
    return {};
  }

  return gitsBasePath / "Launcher" / REGISTRY_KEYS_FILE_NAME;
}
#endif

bool Context::IsPlayback() {
  auto& baseContext = BaseContext::GetBaseInstance();
  return baseContext.SelectedAction == GitsAction::PLAYBACK;
}

bool Context::IsCapture() {
  auto& baseContext = BaseContext::GetBaseInstance();
  return baseContext.SelectedAction == GitsAction::CAPTURE;
}

void Context::ChangeMode(GitsAction gitsAction) {
  auto& baseContext = BaseContext::GetBaseInstance();
  baseContext.SelectedAction = gitsAction;
  //based on selected action / API
  BtnsSideBar->SetVisible(Context::SideBarItem::LABEL_STREAM, IsPlayback());
  BtnsSideBar->SetVisible(Context::SideBarItem::STATS, IsPlayback());
  BtnsSideBar->SetVisible(Context::SideBarItem::RESOURCE_DUMP,
                          IsPlayback() && baseContext.GetStreamAPI() == Api::DIRECTX);
  // TODO Module entries are unconditionally visible. Gating them the way
  // RESOURCE_DUMP is gated belongs to the module itself, once a module can
  // declare which APIs and modes it supports.

  BtnsSideBar->SelectEntry(Context::SideBarItem::OPTIONS);

  UpdateFixedLauncherArguments();
  gits::gui::gui_actions::LoadConfigFromMemory(gitsAction);
  baseContext.SetGITSLogText("");

  auto& currentModeConfiguration = baseContext.ConfigurationForAction();

  currentModeConfiguration.ModifiedGitsConfigurationStr =
      GetYamlStringFromConfig(currentModeConfiguration);

  EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ModeChanged);
}

Context::Context() : BtnsSideBar(nullptr) {
#ifdef _WIN32
  IsAdminMode = IsElevated();
#endif
}

} // namespace gits::gui
