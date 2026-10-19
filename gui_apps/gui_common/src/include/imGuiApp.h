// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <optional>
#include <string>
#include "imgui.h"

#include "baseEventBus.h"
#include "baseConfig.h"
#include "vkWindow.h"
#include "windowEvents.h"

namespace gits::gui {

class ImGuiApp : public WindowEvents {
public:
  ImGuiApp(int argc, char** argv);

#ifdef _WIN32
  bool Initialize(std::string appName, WORD appIcon, const wchar_t* windowTitle);
#else
  bool Initialize(std::string appName, const wchar_t* windowTitle);
#endif
  bool RenderFrame();
  void Shutdown();

public: // WindowEvents
  void OnResize(ImVec2 size) override;
  void OnPosition(ImVec2 position) override;

public: // EventBus Callbacks
  void PathCallback(const Event& e);

private: // this need to be overridden by derived class
  virtual BaseConfig* InitializeApp() {
    return nullptr;
  };
  virtual void CreateUI(){};
  virtual void RenderUI(){};
  virtual void DestroyUI(){};

protected:
  int m_argc;
  char** m_argv;
  bool m_ShouldQuit = false;
  BaseConfig* m_AppConfig = nullptr;
  VKWindow* m_VKWindow = nullptr;
  std::optional<float> m_UpdateUIScale;

  ImVec4 m_ClearColor = ImVec4(0.0, 0.0, 0.0, 1.0);
};

} // namespace gits::gui
