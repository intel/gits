// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiApp.h"

#include "imGuiHelper.h"
#include "baseEventBus.h"
#include "baseContext.h"
#include "log.h"

namespace gits::gui {
ImGuiApp::ImGuiApp(int argc, char** argv) : m_argc(argc), m_argv(argv) {
  // nothing
}

#ifdef _WIN32
bool ImGuiApp::Initialize(std::string appName, WORD appIcon, const wchar_t* windowTitle) {
#else
// The app icon is not yet implemented for Linux
bool ImGuiApp::Initialize(std::string appName, const wchar_t* windowTitle) {
#endif
  gits::log::Initialize(gits::LogLevel::INFO);

  m_AppConfig =
      InitializeApp(); // callback to derived class to initialize m_AppConfig and app-specific members

#ifdef _WIN32
  m_VKWindow = new VKWindow(m_AppConfig, this, appIcon, windowTitle);
#else
  m_VKWindow = new VKWindow(m_AppConfig, this, windowTitle);
#endif

  if (!m_VKWindow->Setup()) {
    LOG_ERROR << "Failed to setup VKWindow";
    return false;
  }

  LOG_DEBUG << "Attempting to restore window size and position from last session: "
            << ImGuiHelper::ToStr(m_AppConfig->WindowPos) << "@"
            << ImGuiHelper::ToStr(m_AppConfig->WindowSize);

  // Load style

  CreateUI(); // callback to derived class to create UI

  m_AppConfig->UIScale = 1.0f;

  auto& context = BaseContext::GetBaseInstance();
  context.UpdatePalette();
  ImGuiHelper::UpdateUIScaling(m_AppConfig->UIScale);
  auto idx = m_AppConfig->Theme.CurrentThemeIdx;
  m_AppConfig->Theme.SetThemeByIdx(idx);
  m_AppConfig->Theme.ApplyTheme();

  EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ThemeChanged);

  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&ImGuiApp::PathCallback, this, std::placeholders::_1));

  m_VKWindow->RestoreWindow();

  return true;
}

bool ImGuiApp::RenderFrame() {
  if (m_UpdateUIScale.has_value()) {
    if (!gits::ImGuiHelper::UpdateUIScaling(gits::ImGuiHelper::CurrentUIScale() +
                                            m_UpdateUIScale.value())) {
      LOG_INFO << "Problem setting new UI scale";
    }
    m_AppConfig->UIScale = gits::ImGuiHelper::CurrentUIScale();
    m_AppConfig->Theme.ApplyTheme();
    m_UpdateUIScale.reset();
  }

  if (m_VKWindow->ProcessEvents() || m_ShouldQuit) {
    return true;
  }

  m_VKWindow->NewFrame();

  EventBus::GetInstance().processEvents();

  RenderUI(); // callback to derived class to render UI

  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();
  const bool minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
  if (!minimized) {
    m_VKWindow->RenderFrame(draw_data, m_ClearColor);
  }

  return false;
}

#pragma region WindowEvents
void ImGuiApp::OnResize(ImVec2 size) {
  m_AppConfig->WindowSize = size;
}

void ImGuiApp::OnPosition(ImVec2 position) {
  m_AppConfig->WindowPos = position;
}
#pragma endregion

void ImGuiApp::Shutdown() {
  DestroyUI(); // callback to derived class to destroy UI

  if (!m_AppConfig->ToFile()) {
    LOG_ERROR << "Failed to save GUIConfig to file " << m_AppConfig->GetGITSBaseConfigPath();
  }

  m_VKWindow->CleanUp();
}

void ImGuiApp::PathCallback(const Event& e) {
  const auto& pathEvent = static_cast<const PathEvent&>(e);
  auto& context = BaseContext::GetBaseInstance();
  auto config = context.GetBaseConfiguration();
  if (config == nullptr) {
    return;
  }

  if (pathEvent.EventType == Path::CAPTURE_TARGET && pathEvent.Mode == GitsAction::CAPTURE) {
    auto path = context.GetPath(Path::CAPTURE_TARGET, GitsAction::CAPTURE);
    if (path.has_value() && !path.value().empty()) {
      config->AddRecentTarget(path.value());
    }
  } else if (pathEvent.EventType == Path::INPUT_STREAM && pathEvent.Mode == GitsAction::PLAYBACK) {
    auto path = context.GetPath(Path::INPUT_STREAM, GitsAction::PLAYBACK);
    if (path.has_value() && !path.value().empty()) {
      config->AddRecentStream(path.value());
    }
  }
};

} // namespace gits::gui
