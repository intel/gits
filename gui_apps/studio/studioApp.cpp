// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "studioApp.h"
#include "imgui.h"
#include "guiActions.h"

#include "studioContext.h"
#include "moduleRegistry.h"

namespace gits::gui {
BaseConfig* StudioApp::InitializeApp() {
  auto& context = StudioContext::GetInstance();

  m_StudioConfig = new StudioConfig();
  m_StudioConfig->FillFromFile();
  context.StudioConfiguration = m_StudioConfig;

  return m_StudioConfig;
};

void StudioApp::CreateUI() {
  auto& context = StudioContext::GetInstance();

  context.SetPath("", Path::CONFIG, GitsAction::CAPTURE, false);
  context.SetPath("", Path::CONFIG, GitsAction::PLAYBACK, false);
  context.SetPath("", Path::CONFIG, GitsAction::SUBCAPTURE, false);
  context.SetPath("", Path::INPUT_STREAM, GitsAction::PLAYBACK, false);
  context.SetPath("", Path::CAPTURE_TARGET, GitsAction::CAPTURE, false);

  context.ResetBasePaths();

  mainWindow = std::make_shared<MainWindow>();
  context.TheMainWindow = mainWindow;

  auto& playHead = context.PlayHeadState;
  playHead.TotalFrames = 0;
  playHead.CurrentFrame = 0;
  playHead.MarkerStart = 0;
  playHead.MarkerEnd = 0;

  context.SendAllPathsSetEvents();

  auto& registry = ModuleRegistry::GetInstance();
  // for now it's only one module in here, so let's just acticvate it...
  if (registry.Modules().size() == 1) {
    //registry.BtnsModules->SelectEntry(ModuleId::Rtsr);
    registry.EnableModule(registry.Modules()[0]->GetID());
  }
};

void StudioApp::RenderUI() {
  auto& context = StudioContext::GetInstance();
  context.Playback.Update();
  context.PlayHeadState.CurrentFrame = context.Playback.Frame();

  const auto& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(io.DisplaySize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

  ImGui::Begin("Full Window", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_MenuBar);

  try {
    mainWindow->RenderMainMenu();
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

void StudioApp::DestroyUI() {
  StudioContext::GetInstance().Playback.Shutdown();
  mainWindow.reset();
  ModuleRegistry::GetInstance().DestroyModules();
};

void StudioApp::PathCallback(const Event& e) {
  const auto& pathEvent = static_cast<const PathEvent&>(e);
  auto& context = StudioContext::GetInstance();
  if (pathEvent.EventType == Path::CAPTURE_TARGET && pathEvent.Mode == GitsAction::CAPTURE) {
    auto path = context.GetPath(Path::CAPTURE_TARGET, GitsAction::CAPTURE);
    if (path.has_value() && !path.value().empty()) {
      if (!context.IsLiveMode()) {
        context.ToogleLiveStreamMode();
      }
    }
  } else if (pathEvent.EventType == Path::INPUT_STREAM && pathEvent.Mode == GitsAction::PLAYBACK) {
    auto path = context.GetPath(Path::INPUT_STREAM, GitsAction::PLAYBACK);
    if (path.has_value() && !path.value().empty()) {
      if (context.IsLiveMode()) {
        context.ToogleLiveStreamMode();
      }
    }
  }
};
} // namespace gits::gui
