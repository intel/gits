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
#include "eventBus.h"

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

std::optional<std::filesystem::path> Context::GetPath(Path pathType,
                                                      std::optional<Mode> appMode) const {
  bool noMode = !appMode.has_value();
  Mode mode = appMode.has_value() ? appMode.value() : AppMode;

  switch (pathType) {

  // universal paths
  case Path::GITS_BASE:
    return Paths.BasePath;
  case Path::CUSTOM_PLAYER:
    return Paths.CustomPlayerPath;

  // unique paths - independent of mode
  case Path::SCREENSHOTS:
    if (noMode || mode == Mode::PLAYBACK) {
      return Paths.Playback.ScreenshotsPath;
    }
    return std::nullopt;
  case Path::TRACE:
    if (noMode || mode == Mode::PLAYBACK) {
      return Paths.Playback.TracePath;
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
    default:
      return std::nullopt;
    }
    break;
  case Mode::CAPTURE:
    switch (pathType) {
    case Path::CONFIG:
      return Paths.Capture.ConfigPath;
    case Path::OUTPUT_STREAM:
      return Paths.Capture.OutputStreamPath;
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
      return Paths.Subcapture.OutputStreamPath;
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
  if (pathType == Path::GITS_BASE) {
    Paths.BasePath = path;
    result = true;
  } else {
    switch (mode) {
    case Mode::PLAYBACK:
      switch (pathType) {
      case Path::CUSTOM_PLAYER:
        Paths.CustomPlayerPath = path;
        result = true;
        break;
      case Path::CONFIG:
        Paths.Playback.ConfigPath = path;
        result = true;
        break;
      case Path::INPUT_STREAM:
        Paths.Playback.InputStreamPath = path;
        result = true;
        break;
      case Path::SCREENSHOTS:
        Paths.Playback.ScreenshotsPath = path;
        result = true;
        break;
      case Path::TRACE:
        Paths.Playback.TracePath = path;
        result = true;
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
        Paths.Capture.OutputStreamPath = path;
        result = true;
        break;
      default:
        result = false;
        break;
      }
      break;
    case Mode::SUBCAPTURE:
      switch (pathType) {
      case Path::CUSTOM_PLAYER:
        Paths.CustomPlayerPath = path;
        result = true;
        break;
      case Path::CONFIG:
        Paths.Subcapture.ConfigPath = path;
        result = true;
        break;
      case Path::INPUT_STREAM:
        Paths.Subcapture.InputStreamPath = path;
        result = true;
        break;
      case Path::OUTPUT_STREAM:
        Paths.Subcapture.OutputStreamPath = path;
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
    PathEvent event;
    event.EventType = pathType;
    event.Mode = mode;
    EventBus::GetInstance().publish<PathEvent>(event);
  }

  return result;
}

std::filesystem::path Context::GetGITSPlayerPath() const {
  if (UseCustomGITSPlayer) {
    return Paths.CustomPlayerPath;
  } else {
    return Paths.BasePath / "Player" / "gitsPlayer.exe";
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
  //based on app mode
  BtnsSideBar->SetEnabled(Context::SideBarItem::OPTIONS, IsPlayback());
  BtnsSideBar->SetEnabled(Context::SideBarItem::STATS, IsPlayback());

  switch (mode) {
  case Mode::PLAYBACK:
    BtnsSideBar->SelectEntry(Context::SideBarItem::OPTIONS);
    break;
  case Mode::CAPTURE:
    BtnsSideBar->SelectEntry(Context::SideBarItem::CONFIG);
    break;
  case Mode::SUBCAPTURE:
    BtnsSideBar->SelectEntry(Context::SideBarItem::CONFIG);
    break;
  };

  UpdateFixedLauncherArguments();
  gits::gui::UpdateCLICall();
  gits::gui::LoadConfigFile();
  GITSLogEditor->SetText("");
}

void Context::UpdatePalette() {
  if (ConfigEditor) {
    ConfigEditor->UpdatePalette();
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
