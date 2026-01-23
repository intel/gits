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

#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"

#include "fileActions.h"
#include "launcherConfig.h"
#include "log.h"
#include "launcherActions.h"
#include "launcherFormatter.h"
#include "mainWindow.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>

namespace gits::gui {

void Context::UpdateFixedLauncherArguments() {
  FixedLauncherArguments = "";
  if (AppMode == Mode::PLAYBACK) {
    if (EasyOptionsPanel) {
      FixedLauncherArguments += EasyOptionsPanel->GetCLIArguments();
    }
  }
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

std::filesystem::path Context::GetPath(Paths path) const {
  switch (path) {
  case Paths::STREAM:
    return StreamPath;
  case Paths::TARGET:
    return TargetPath;
  case Paths::GITS_PLAYER:
    if (UseCustomGITSPlayer) {
      return GITSPlayerPath;
    } else {
      return GITSBasePath / "Player" / "gitsPlayer.exe";
    }
  case Paths::GITS_BASE:
    return GITSBasePath;
  case Paths::CONFIG:
    return ConfigPath;
  case Paths::CAPTURE_CONFIG:
    return CaptureConfigPath;
  case Paths::CAPTURE_OUTPUT:
    return CaptureOutputPath;
  default:
    return std::filesystem::path();
  }
}

bool Context::IsPlayback() {
  return AppMode == Mode::PLAYBACK;
}

bool Context::IsCapture() {
  return AppMode == Mode::CAPTURE;
}

void Context::ChangeMode(Mode mode) {
  AppMode = mode;
  switch (mode) {
  case Mode::PLAYBACK:
    CurrentMainAction = gui::Context::MainAction::PLAYBACK;
    break;
  case Mode::CAPTURE:
    CurrentMainAction = gui::Context::MainAction::CAPTURE;
    break;
  case Mode::SUBCAPTURE:
    CurrentMainAction = gui::Context::MainAction::SUBCAPTURE;
    break;
  };
  BtnsSideBar->SetEnabled(Context::SideBarItems::OPTIONS, IsPlayback());
  BtnsSideBar->SetEnabled(Context::SideBarItems::STATS, IsPlayback());
  if (!BtnsSideBar->IsEnabled(BtnsSideBar->Selected())) {
    BtnsSideBar->SelectEntry(Context::SideBarItems::CONFIG);
  }
  UpdateFixedLauncherArguments();
  gits::gui::UpdateCLICall(*this);
  gits::gui::LoadConfigFile(this);
  GITSLogEditor->SetText("");
}

void Context::UpdatePalette() {
  if (ConfigEditor) {
    ConfigEditor->UpdatePalette();
  }
  if (CLIEditor) {
    CLIEditor->UpdatePalette();
  }
  if (LogEditor) {
    LogEditor->UpdatePalette();
  }
  if (GITSLogEditor) {
    GITSLogEditor->UpdatePalette();
  }
  if (TraceInfoEditor) {
    TraceInfoEditor->UpdatePalette();
  }
  if (DiagsEditor) {
    DiagsEditor->UpdatePalette();
  }
  if (TraceConfigEditor) {
    TraceConfigEditor->UpdatePalette();
  }
  if (TraceStatsEditor) {
    TraceStatsEditor->UpdatePalette();
  }
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
