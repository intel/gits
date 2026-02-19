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
  glfwSetWindowPos(m_GLFWWindow, m_Context.LauncherConfiguration.WindowPos.x,
                   m_Context.LauncherConfiguration.WindowPos.y);
  glfwSetWindowSize(m_GLFWWindow, m_Context.LauncherConfiguration.WindowSize.x,
                    m_Context.LauncherConfiguration.WindowSize.y);
#endif
}

void GUIController::Resized(int width, int height) {
  ImVec2 size(width, height);
#ifdef _WIN32
  RECT rect;
  GetWindowRect(m_Handle, &rect);
  size = ImVec2(static_cast<float>(rect.right - rect.left),
                static_cast<float>(rect.bottom - rect.top));
  m_Context.LauncherConfiguration.WindowSize = size;
#endif
  m_Context.LauncherConfiguration.WindowSize = size;
}

void GUIController::Positioned(int x, int y) {
  ImVec2 position(x, y);
#ifdef _WIN32
  RECT rect;
  GetWindowRect(m_Handle, &rect);
  position = ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top));
#endif
  m_Context.LauncherConfiguration.WindowPos = position;
}

void GUIController::UpdateUIScale() {
  if (!m_UpdateUIScale.has_value()) {
    return;
  }

  if (!gits::ImGuiHelper::UpdateUIScaling(gits::ImGuiHelper::CurrentUIScale() +
                                          m_UpdateUIScale.value())) {
    LOG_INFO << "Problem setting new UI scale";
  }
  m_Context.LauncherConfiguration.UIScale = gits::ImGuiHelper::CurrentUIScale();
  m_UpdateUIScale.reset();
}

void GUIController::RenderUI() {
  m_Context.ImguiIDs = 0;
  try {
    mainWindow->Render();
    FileDialogs(m_Context);
  } catch (const std::exception& e) {
    ImGui::Text("UI Rendering Error: %s", e.what());
  }
  if (m_Context.UIScaleDelta.has_value()) {
    m_UpdateUIScale = m_Context.UIScaleDelta;
    m_Context.UIScaleDelta = std::nullopt;
  }
  ShouldQuit = m_Context.ShouldQuit;
}

void GUIController::SetupGui() {
  m_Context.LauncherConfiguration = LauncherConfig::FromFile();

  m_Context.GITSPlayerPath = m_Context.LauncherConfiguration.GITSPlayerPath;
  m_Context.GITSBasePath = m_Context.LauncherConfiguration.GITSBasePath;
  m_Context.StreamPath = m_Context.LauncherConfiguration.StreamPath;
  m_Context.TargetPath = m_Context.LauncherConfiguration.TargetPath;
  m_Context.ConfigPath = m_Context.LauncherConfiguration.ConfigPath;
  m_Context.CaptureConfigPath = m_Context.LauncherConfiguration.CaptureConfigPath;
  m_Context.CustomArguments = m_Context.LauncherConfiguration.CustomArguments;
  m_Context.CaptureCustomArguments = m_Context.LauncherConfiguration.CaptureCustomArguments;
  m_Context.CaptureOutputPath = m_Context.LauncherConfiguration.CaptureOutputPath;

  LOG_INFO << "Attempting to restore window size and position from last session: "
           << ImGuiHelper::ToStr(m_Context.LauncherConfiguration.WindowPos) << "@"
           << ImGuiHelper::ToStr(m_Context.LauncherConfiguration.WindowSize);

  ImGuiHelper::UpdateUIScaling(m_Context.LauncherConfiguration.UIScale);

  // Load style
  ImGuiStyle& style = ImGui::GetStyle();
  style.ChildBorderSize = 0.f;
  style.DisabledAlpha = 0.2f;

  // Setup editor
  // - scrollbar for max width of file, not visible part
  m_Context.ConfigEditor = std::make_unique<TextEditorWidget>("ConfigEditor");
  m_Context.ConfigEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.ConfigEditor->GetEditor().SetTabSize(4);
  m_Context.ConfigEditor->SetCheckCallback(&ValidateGITSConfig);

  m_Context.CLIEditor = std::make_unique<TextEditorWidget>("CLIEditor");
  m_Context.CLIEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  m_Context.CLIEditor->GetEditor().SetReadOnly(true);
  m_Context.CLIEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.CLIEditor->GetEditor().SetTabSize(4);

  m_Context.GITSLogEditor = std::make_unique<TextEditorWidget>("GITSLogEditor");
  m_Context.GITSLogEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  m_Context.GITSLogEditor->GetEditor().SetReadOnly(true);
  m_Context.GITSLogEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.GITSLogEditor->GetEditor().SetTabSize(4);

  m_Context.LogEditor = std::make_unique<TextEditorWidget>("LogEditor");
  m_Context.LogEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  m_Context.LogEditor->GetEditor().SetReadOnly(true);
  m_Context.LogEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.LogEditor->GetEditor().SetTabSize(4);

  m_Context.DiagsEditor = std::make_unique<TextEditorWidget>("DiagsEditor");
  m_Context.DiagsEditor->GetEditor().SetReadOnly(true);
  m_Context.DiagsEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.DiagsEditor->GetEditor().SetTabSize(4);
  m_Context.DiagsEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  m_Context.DiagsEditor->GetEditor().SetColorizerEnable(false);

  m_Context.TraceConfigEditor = std::make_unique<TextEditorWidget>("TraceConfigEditor");
  m_Context.TraceConfigEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  m_Context.TraceConfigEditor->GetEditor().SetReadOnly(true);
  m_Context.TraceConfigEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.TraceConfigEditor->GetEditor().SetTabSize(4);
  m_Context.TraceConfigEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  m_Context.TraceConfigEditor->GetEditor().SetColorizerEnable(false);

  m_Context.TraceStatsEditor = std::make_unique<TextEditorWidget>("TraceStatsEditor");
  m_Context.TraceStatsEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  m_Context.TraceStatsEditor->GetEditor().SetReadOnly(true);
  m_Context.TraceStatsEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.TraceStatsEditor->GetEditor().SetTabSize(4);
  m_Context.TraceStatsEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});
  m_Context.TraceStatsEditor->GetEditor().SetColorizerEnable(false);

  m_Context.EasyOptionsPanel = std::make_unique<EzOptionsPanel>(m_Context);

  m_Context.MetaDataPanel = std::make_unique<MetaDataPanel>(m_Context);

  m_Context.LogAppender = std::make_unique<TextEditorAppender>(m_Context.LogEditor.get());

  plog::get()->addAppender(m_Context.LogAppender.get());

  m_Context.BtnsSideBar = new ImGuiHelper::TabGroup(Labels::SIDE_BAR(), false, true);
  m_Context.BtnsAPI = new ImGuiHelper::TabGroup(Labels::CONFIG_SECTIONS(), true, false);
  m_Context.BtnsMetaData = new ImGuiHelper::TabGroup(Labels::META_DATA(), true, false);

  m_Context.BtnsSideBar->SelectEntry(Context::SideBarItems::CONFIG);

  SetImGuiStyle(&m_Context, m_Context.LauncherConfiguration.Theme.CurrentThemeIdx);

  LoadConfigFile(&m_Context);

  m_Context.TheMainWindow = std::make_shared<MainWindow>(m_Context);
  mainWindow.reset(m_Context.TheMainWindow.get());

  UpdateCLICall(m_Context);

  // Initialize panels in the desired startup state
  m_Context.ChangeMode(Context::Mode::CAPTURE);
  m_Context.BtnsSideBar->SelectEntry(Context::SideBarItems::CONFIG);
}

void GUIController::TeardownGui() {
  m_Context.LauncherConfiguration.GITSPlayerPath = m_Context.GITSPlayerPath.string();
  m_Context.LauncherConfiguration.GITSBasePath = m_Context.GITSBasePath.string();
  m_Context.LauncherConfiguration.StreamPath = m_Context.StreamPath.string();
  m_Context.LauncherConfiguration.TargetPath = m_Context.TargetPath.string();
  m_Context.LauncherConfiguration.ConfigPath = m_Context.ConfigPath.string();
  m_Context.LauncherConfiguration.CaptureConfigPath = m_Context.CaptureConfigPath.string();
  m_Context.LauncherConfiguration.CustomArguments = m_Context.CustomArguments;
  m_Context.LauncherConfiguration.CaptureCustomArguments = m_Context.CaptureCustomArguments;
  m_Context.LauncherConfiguration.CaptureOutputPath = m_Context.CaptureOutputPath;

  if (!m_Context.LauncherConfiguration.ToFile()) {
    LOG_ERROR << "Failed to save LauncherConfig to file "
              << m_Context.LauncherConfiguration.GetGITSLauncherConfigPath();
  }

  m_Context.ConfigEditor.reset();
  m_Context.LogEditor.reset();
  m_Context.CLIEditor.reset();

  delete m_Context.BtnsSideBar;
  m_Context.BtnsSideBar = nullptr;
  delete m_Context.BtnsAPI;
  m_Context.BtnsAPI = nullptr;
  mainWindow.release();
}

void GUIController::DestroyGui() {
  m_Context.LogAppender.reset();
}

} // namespace gits::gui
