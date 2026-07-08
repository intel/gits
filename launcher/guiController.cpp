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
#include "playbackOptionsPanel.h"
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

  // Read config, store paths from config, ensure paths are correct or detect them
  context.LauncherConfiguration = LauncherConfig::FromFile();
  if (!DetectBasePaths()) {
    context.Paths = context.LauncherConfiguration.Paths;
  } else {
    context.Paths.Capture = context.LauncherConfiguration.Paths.Capture;
    context.Paths.Playback = context.LauncherConfiguration.Paths.Playback;
    context.Paths.Subcapture = context.LauncherConfiguration.Paths.Subcapture;
  }

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
  auto modifiedEditorConfig = TextEditorWidget::Config{
      .ShowToolbar = true,
      .ToolBarItems = {
          TextEditorWidget::TOOL_BAR_ITEMS::SAVE, TextEditorWidget::TOOL_BAR_ITEMS::REVERT,
          TextEditorWidget::TOOL_BAR_ITEMS::UNDO, TextEditorWidget::TOOL_BAR_ITEMS::REDO,
          TextEditorWidget::TOOL_BAR_ITEMS::CHECK, TextEditorWidget::TOOL_BAR_ITEMS::EXPORT}};

  // The save button for the modified config will save the changes to the in-memory struct
  // by serializing the text inside into the struct
  const auto modifiedConfigOnSaveCallback = [](std::filesystem::path configPath) {
    auto& context = Context::GetInstance();
    auto& currentModeConfiguration = context.ConfigurationForMode();

    std::string text = currentModeConfiguration.ModifiedGitsConfigurationStr;

    YAML::Node yaml = YAML::Load(text);
    if (gits::Configurator::LoadInto(yaml, &currentModeConfiguration.ModifiedGitsConfiguration)) {
      if (yaml["Overrides"]) {
        currentModeConfiguration.ModifiedOverrides = yaml["Overrides"];
      }
    } else {
      LOG_ERROR << "Couldn't serialize GITS configuration from the YAML editor";
    }

    context.UpdateInMemoryConfig();
  };

  const auto modifiedConfigOnExportCallback = [](std::filesystem::path filePath) {
    auto& context = Context::GetInstance();
    auto exportPath = std::filesystem::path();
    switch (context.AppMode) {
    case Mode::CAPTURE:
      exportPath = context.GetPathSafe(Path::CAPTURE_TARGET, Mode::CAPTURE);
      break;
    case Mode::PLAYBACK:
      exportPath = context.GetGITSPlayerPath();
      break;
    case Mode::SUBCAPTURE:
      exportPath = context.GetGITSPlayerPath();
      break;
    default:
      break;
    }
    if (!exportPath.empty()) {
      exportPath = exportPath.parent_path();
      auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::stringstream ss;
      ss << "gits_config_" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d_%H-%M-%S")
         << ".yml";
      exportPath /= ss.str();
    }
    ShowFileDialog(FileDialogKey{.Path = Path::CONFIG_EXPORT, .Mode = context.AppMode}, exportPath);
  };

  context.GITSLogEditor = std::make_unique<TextEditorWidget>("GITSLogEditor");
  auto logConfig =
      TextEditorWidget::Config{.ShowToolbar = true,
                               .ToolBarItems = {TextEditorWidget::TOOL_BAR_ITEMS::EXPORT,
                                                TextEditorWidget::TOOL_BAR_ITEMS::SEND_BY_EMAIL}};
  context.GITSLogEditor->SetConfig(logConfig);
  context.GITSLogEditor->GetEditor().SetReadOnly(true);
  context.GITSLogEditor->GetEditor().SetShowWhitespaces(false);
  context.GITSLogEditor->GetEditor().SetTabSize(4);
  const auto gitsLogOnExportCallback = [](std::filesystem::path logPath) {
    auto& context = Context::GetInstance();
    switch (context.AppMode) {
    case Mode::CAPTURE:
      logPath = context.GetPathSafe(Path::OUTPUT_STREAM, Mode::CAPTURE);
      break;
    case Mode::PLAYBACK:
      logPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
      break;
    case Mode::SUBCAPTURE:
      logPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::SUBCAPTURE);
      break;
    }
    if (!logPath.empty()) {
      logPath = logPath.parent_path();
      auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::stringstream ss;
      ss << "gits_log_" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d_%H-%M-%S") << ".txt";
      logPath /= ss.str();
    }
    ShowFileDialog(FileDialogKey{.Path = Path::GITS_LOG, .Mode = context.AppMode}, logPath);
  };
  context.GITSLogEditor->SetExportCallback(gitsLogOnExportCallback);
  context.GITSLogEditor->SetSendByEmailCallback([](const std::string& logText) {
    SendLogByEmail(Labels::EMAIL_LOG_RECIPIENT, Labels::EMAIL_LOG_SUBJECT,
                   CreateEmailBodyWithLog(logText));
  });

  context.LogEditor = std::make_unique<TextEditorWidget>("LogEditor");
  context.LogEditor->SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  context.LogEditor->GetEditor().SetReadOnly(true);
  context.LogEditor->GetEditor().SetShowWhitespaces(false);
  context.LogEditor->GetEditor().SetTabSize(4);

  context.LogAppender = std::make_unique<TextEditorAppender>(context.LogEditor.get());

  plog::get()->addAppender(context.LogAppender.get());

#ifdef _WIN32
  LOG_INFO << Labels::LOG_LAUNCHER_ADMIN_MODE_PREFIX
           << (context.IsAdminMode ? Labels::LOG_LAUNCHER_ADMIN_MODE_YES
                                   : Labels::LOG_LAUNCHER_ADMIN_MODE_NO);
#endif

  context.BtnsSideBar =
      new ImGuiHelper::TabGroup<Context::SideBarItem>(Labels::SIDE_BAR(), false, true);

  SetImGuiStyle(context.LauncherConfiguration.Theme.CurrentThemeIdx);

  context.TheMainWindow = std::make_shared<MainWindow>();
  mainWindow = context.TheMainWindow;

  // Initialize panels and UI in the desired startup state
  context.ChangeMode(Mode::CAPTURE);
  context.SetCaptureAPI(Api::DIRECTX);

  context.SendAllPathsSetEvents();

  LoadConfigFile(context.AppMode);
}

void GUIController::TeardownGui() {
  auto& context = Context::GetInstance();
  context.LauncherConfiguration.Paths = context.Paths;

  if (!context.LauncherConfiguration.ToFile()) {
    LOG_ERROR << "Failed to save LauncherConfig to file "
              << context.LauncherConfiguration.GetGITSLauncherConfigPath();
  }

  context.LogEditor.reset();

  delete context.BtnsSideBar;
  context.BtnsSideBar = nullptr;
  mainWindow.reset();
}

MainWindow* GUIController::GetMainWindow() const {
  return mainWindow.get();
}

void GUIController::DestroyGui() {
  Context::GetInstance().LogAppender.reset();
}

} // namespace gits::gui
