// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helloPanel.h"

#include "moduleRegistry.h"
#include "baseContext.h"
#include "baseEventBus.h"
#include "log.h"

#include <imgui.h>

namespace gits::gui {

namespace {

constexpr const char* HELLO_NAME = "Hello";
constexpr const char* HELLO_NAME_HINT = "Minimal Studio module";

const char* ActionName(GitsAction action) {
  switch (action) {
  case GitsAction::CAPTURE:
    return "Capture";
  case GitsAction::PLAYBACK:
    return "Playback";
  case GitsAction::SUBCAPTURE:
    return "Subcapture";
  }
  return "Unknown";
}

} // namespace

HelloPanel::HelloPanel() {
  // Subscribed once, for the panel's whole life. EventBus::unsubscribe cannot match a lambda.
  EventBus::GetInstance().subscribe<AppEvent>([this](const Event&) { ++m_ThemeChanges; },
                                              {AppEvent::Type::ThemeChanged});
}

void HelloPanel::Render() {
  ImGui::TextUnformatted("Hello from a Studio module DLL.");
  ImGui::Text("Active: %s", m_Active ? "yes" : "no");
  ImGui::Text("Studio action: %s", ActionName(BaseContext::GetBaseInstance().SelectedAction));
  ImGui::Text("Theme changes received from Studio: %d", m_ThemeChanges);
}

const char* HelloPanel::GetName() const {
  return HELLO_NAME;
}

const char* HelloPanel::GetNameHint() const {
  return HELLO_NAME_HINT;
}

bool HelloPanel::Activate() {
  m_Active = true;
  LOG_INFO << "Hello module activated";
  return true;
}

bool HelloPanel::Deactivate() {
  m_Active = false;
  LOG_INFO << "Hello module deactivated";
  return true;
}

bool HelloPanel::IsActive() {
  return m_Active;
}

} // namespace gits::gui

GITS_GUI_REGISTER_MODULE("hello",
                         ::gits::gui::HelloPanel,
                         ::gits::gui::HELLO_NAME,
                         ::gits::gui::HELLO_NAME_HINT)
