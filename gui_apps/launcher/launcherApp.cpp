// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherApp.h"

#include "resource.h"
#include "labels.h"
#include "imGuiHelper.h"
#include "context.h"
#include "guiActions.h"

namespace gits::gui {

LauncherConfig* LauncherApp::InitializeApp() {
  auto& context = Context::GetInstance();

  m_LauncherConfig = new LauncherConfig();
  m_LauncherConfig->FillFromFile();
  context.LauncherConfiguration = m_LauncherConfig;
  context.CurrentMode = m_LauncherConfig->LastMode;

  return m_LauncherConfig;
};

void LauncherApp::CreateUI() {
  auto& context = Context::GetInstance();

  context.DetectBasePaths();

  SetAllConfigsFromBasePath();

#ifdef _WIN32
  LOG_INFO << Labels::LOG_LAUNCHER_ADMIN_MODE_PREFIX
           << (context.IsAdminMode ? Labels::LOG_LAUNCHER_ADMIN_MODE_YES
                                   : Labels::LOG_LAUNCHER_ADMIN_MODE_NO);
#endif

  context.BtnsSideBar =
      new ImGuiHelper::TabGroup<Context::SideBarItem>(Labels::SIDE_BAR(), false, true, true);

  context.TheMainWindow = std::make_shared<MainWindow>();
  mainWindow = context.TheMainWindow;

  // Initialize panels and UI in the desired startup state
  context.ChangeMode(GitsAction::CAPTURE);
  context.SetCaptureAPI(context.SelectedApiForCapture);

  context.SendAllPathsSetEvents();

  gui_actions::LoadConfigFile(context.SelectedAction);
};

void LauncherApp::RenderUI() {
  auto& context = Context::GetInstance();

  const auto& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(io.DisplaySize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

  ImGui::Begin("Full Window", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_MenuBar);

  try {
    mainWindow->Render();
    gui_actions::FileDialogs();
  } catch (const std::exception& e) {
    ImGui::Text("UI Rendering Error: %s", e.what());
  }
  if (context.UIScaleDelta.has_value()) {
    m_UpdateUIScale = context.UIScaleDelta;
    context.UIScaleDelta = std::nullopt;
  }
  m_ShouldQuit = context.ShouldQuit;

  ImGui::End();
  ImGui::PopStyleVar();
};

void LauncherApp::DestroyUI() {
  auto& context = Context::GetInstance();

  delete context.BtnsSideBar;
  context.BtnsSideBar = nullptr;
  mainWindow.reset();
};

void LauncherApp::OnFileDrop(const std::filesystem::path& filePath) {
  gits::gui::FileDropEvent event{std::filesystem::path(filePath)};
  gits::gui::EventBus::GetInstance().publish(event);
}
} // namespace gits::gui
