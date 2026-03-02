// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "guiController.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include "imgui.h"

#include "context.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <iostream>
#include <istream>
#include <vector>
#include <map>
#include <cmath>
#include "imgui.h"
#include "ImGuiFileDialog.h"

#include "imGuiHelper.h"
#include "context.h"
#include "launcherConfig.h"
#include "buttonGroup.h"
#include "resource.h"
#include "log.h"

#include "basePanel.h"
#include "ezOptionsPanel.h"
#include "contentPanel.h"
#include "mainWindow.h"
#include "launcherActions.h"
#include "labels.h"
#include "eventBus.h"

#include "imGuiHelper.h"
#include "log.h"

namespace gits::gui {
#ifdef _WIN32
GUIController::GUIController(HWND hwnd) : m_Handle(hwnd), m_CleanUpAfterRecording(true) {
#else
GUIController::GUIController(GLFWwindow* glfwWindow)
    : m_GLFWWindow(glfwWindow), m_CleanUpAfterRecording(true) {
#endif
  mainWindow = nullptr;

  // nothing to do here
}
void GUIController::DrawGui() {
  const auto& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(io.DisplaySize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
  ImGui::Begin("Full Window", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
  RenderUI();
  ImGui::End();
  ImGui::PopStyleVar();
}

void GUIController::RestoreWindow() {
#ifndef _WIN32
  glfwSetWindowPos(m_GLFWWindow, Context::GetInstance().LauncherConfiguration.WindowPos.x,
                   Context::GetInstance().LauncherConfiguration.WindowPos.y);
  glfwSetWindowSize(m_GLFWWindow, Context::GetInstance().LauncherConfiguration.WindowSize.x,
                    Context::GetInstance().LauncherConfiguration.WindowSize.y);
#endif
}

void GUIController::Resized(int width, int height) {
  ImVec2 size(width, height);
#ifdef _WIN32
  RECT rect;
  GetWindowRect(m_Handle, &rect);
  size = ImVec2(static_cast<float>(rect.right - rect.left),
                static_cast<float>(rect.bottom - rect.top));

  Context::GetInstance().LauncherConfiguration.WindowSize = size;
#endif
  Context::GetInstance().LauncherConfiguration.WindowSize = size;
}

void GUIController::Positioned(int x, int y) {
  ImVec2 position(x, y);
#ifdef _WIN32
  RECT rect;
  GetWindowRect(m_Handle, &rect);
  position = ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top));
#endif
  Context::GetInstance().LauncherConfiguration.WindowPos = position;
}

void GUIController::UpdateUIScale() {
  if (!m_UpdateUIScale.has_value()) {
    return;
  }

  if (!gits::ImGuiHelper::UpdateUIScaling(gits::ImGuiHelper::CurrentUIScale() +
                                          m_UpdateUIScale.value())) {
    LOG_INFO << "Problem setting new UI scale";
  }
  Context::GetInstance().LauncherConfiguration.UIScale = gits::ImGuiHelper::CurrentUIScale();
  m_UpdateUIScale.reset();
}

void GUIController::RenderUI() {
  EventBus::GetInstance().processEvents();
  Context::GetInstance().ImguiIDs = 0;
  try {
    mainWindow->Render();
    FileDialogs();
  } catch (const std::exception& e) {
    ImGui::Text("UI Rendering Error: %s", e.what());
  }
  if (Context::GetInstance().UIScaleDelta.has_value()) {
    m_UpdateUIScale = Context::GetInstance().UIScaleDelta;
    Context::GetInstance().UIScaleDelta = std::nullopt;
  }
  ShouldQuit = Context::GetInstance().ShouldQuit;
}

void GUIController::SetupGui() {
  auto& context = Context::GetInstance();
  context.LauncherConfiguration = LauncherConfig::FromFile();

  context.Paths = context.LauncherConfiguration.Paths;
  LOG_INFO << "Attempting to restore window size and position from last session: "
           << ImGuiHelper::ToStr(context.LauncherConfiguration.WindowPos) << "@"
           << ImGuiHelper::ToStr(context.LauncherConfiguration.WindowSize);

  ImGuiHelper::UpdateUIScaling(context.LauncherConfiguration.UIScale);

  // Load style
  ImGuiStyle& style = ImGui::GetStyle();
  style.ChildBorderSize = 0.f;
  style.DisabledAlpha = 0.2f;

  // Setup editor
  // - scrollbar for max width of file, not visible part
  context.ConfigEditor = std::make_unique<TextEditorWidget>("ConfigEditor");
  context.ConfigEditor->GetEditor().SetShowWhitespaces(false);
  context.ConfigEditor->GetEditor().SetTabSize(4);
  context.ConfigEditor->SetCheckCallback(&ValidateGITSConfig);

  context.GITSLogEditor = std::make_unique<TextEditorWidget>("GITSLogEditor");
  context.GITSLogEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  context.GITSLogEditor->GetEditor().SetReadOnly(true);
  context.GITSLogEditor->GetEditor().SetShowWhitespaces(false);
  context.GITSLogEditor->GetEditor().SetTabSize(4);

  context.LogEditor = std::make_unique<TextEditorWidget>("LogEditor");
  context.LogEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  context.LogEditor->GetEditor().SetReadOnly(true);
  context.LogEditor->GetEditor().SetShowWhitespaces(false);
  context.LogEditor->GetEditor().SetTabSize(4);

  context.DiagsEditor = std::make_unique<TextEditorWidget>("DiagsEditor");
  context.DiagsEditor->GetEditor().SetReadOnly(true);
  context.DiagsEditor->GetEditor().SetShowWhitespaces(false);
  context.DiagsEditor->GetEditor().SetTabSize(4);
  context.DiagsEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  context.DiagsEditor->GetEditor().SetColorizerEnable(false);

  context.TraceConfigEditor = std::make_unique<TextEditorWidget>("TraceConfigEditor");
  context.TraceConfigEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  context.TraceConfigEditor->GetEditor().SetReadOnly(true);
  context.TraceConfigEditor->GetEditor().SetShowWhitespaces(false);
  context.TraceConfigEditor->GetEditor().SetTabSize(4);
  context.TraceConfigEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  context.TraceConfigEditor->GetEditor().SetColorizerEnable(false);

  context.TraceStatsEditor = std::make_unique<TextEditorWidget>("TraceStatsEditor");
  context.TraceStatsEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  context.TraceStatsEditor->GetEditor().SetReadOnly(true);
  context.TraceStatsEditor->GetEditor().SetShowWhitespaces(false);
  context.TraceStatsEditor->GetEditor().SetTabSize(4);
  context.TraceStatsEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  context.TraceStatsEditor->GetEditor().SetColorizerEnable(false);

  context.EasyOptionsPanel = std::make_unique<EzOptionsPanel>();

  context.LogAppender = std::make_unique<TextEditorAppender>(context.LogEditor.get());

  plog::get()->addAppender(context.LogAppender.get());

  context.BtnsSideBar = new ImGuiHelper::TabGroup(Labels::SIDE_BAR(), false, true);
  context.BtnsAPI = new ImGuiHelper::TabGroup(Labels::CONFIG_SECTIONS(), true, false);
  context.BtnsMetaData = new ImGuiHelper::TabGroup(Labels::META_DATA(), true, false);

  SetImGuiStyle(context.LauncherConfiguration.Theme.CurrentThemeIdx);

  LoadConfigFile();

  context.TheMainWindow = std::make_shared<MainWindow>();
  mainWindow.reset(context.TheMainWindow.get());

  // Initialize panels in the desired startup state
  Context::GetInstance().Context::GetInstance().ChangeMode(Mode::CAPTURE);
  Context::GetInstance().BtnsSideBar->SelectEntry(Context::SideBarItem::CONFIG);
  UpdateCLICall();
}

void GUIController::TeardownGui() {
  auto& context = Context::GetInstance();
  context.LauncherConfiguration.Paths = context.Paths;

  if (!context.LauncherConfiguration.ToFile()) {
    LOG_ERROR << "Failed to save LauncherConfig to file "
              << context.LauncherConfiguration.GetGITSLauncherConfigPath();
  }

  context.ConfigEditor.reset();
  context.LogEditor.reset();

  delete context.BtnsSideBar;
  context.BtnsSideBar = nullptr;
  delete context.BtnsAPI;
  context.BtnsAPI = nullptr;
  mainWindow.release();
}

MainWindow* GUIController::GetMainWindow() const {
  return mainWindow.get();
}

void GUIController::DestroyGui() {
  Context::GetInstance().LogAppender.reset();
}

} // namespace gits::gui
