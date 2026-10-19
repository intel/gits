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

#include "studioContext.h"
#include "studioEventBus.h"
#include "studioConfig.h"
#include "log.h"
#include "mainWindow.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>

namespace gits::gui {

StudioContext::StudioContext() {
  StudioConfiguration = nullptr;
  m_LiveMode = false;

  SelectedAction = m_LiveMode ? GitsAction::CAPTURE : GitsAction::PLAYBACK;
}

BaseConfig* StudioContext::GetBaseConfiguration() {
  return StudioConfiguration;
}

void StudioContext::ToogleLiveStreamMode() {
  m_LiveMode = !m_LiveMode;
  SelectedAction = m_LiveMode ? GitsAction::CAPTURE : GitsAction::PLAYBACK;

  auto event = StudioEvent(StudioEvent::Type::LiveStreamModeChanged);
  EventBus::GetInstance().publish(event);
}

bool StudioContext::IsLiveMode() const {
  return m_LiveMode;
}

} // namespace gits::gui
