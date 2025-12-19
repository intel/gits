// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <utility>
#include <optional>
#include <iostream>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <GLFW/glfw3.h>
#endif

#include "fileActions.h"
#include "imGuiHelper.h"

#include "context.h"
#include "mainWindow.h"
#include "launcherConfig.h"

namespace gits::gui {
struct Settings {
  static constexpr const char* WINDOW_TITLE = "GITS Launcher";
  static constexpr const wchar_t* WINDOW_TITLE_W = L"GITS Launcher";

  static constexpr auto CLEAR_COLOR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  static void SetVKClearColor(VkClearValue* clearValue) {

    clearValue->color.float32[0] = CLEAR_COLOR.x * CLEAR_COLOR.w;
    clearValue->color.float32[1] = CLEAR_COLOR.y * CLEAR_COLOR.w;
    clearValue->color.float32[2] = CLEAR_COLOR.z * CLEAR_COLOR.w;
    clearValue->color.float32[3] = CLEAR_COLOR.w;
  }
};

class GUIController {
public:
#ifdef _WIN32
  GUIController(HWND hwnd);
#else
  GUIController(GLFWwindow* glfwWindow);
#endif
  void SetupGui();
  void DrawGui();
  void TeardownGui();
  void DestroyGui();
  void Resized(int width = 0, int height = 0);
  void Positioned(int x = 0, int y = 0);
  void RestoreWindow();
  void UpdateUIScale();

  bool ShouldQuit = false;

private:
  void RenderUI();

private:
#ifdef _WIN32
  HWND m_Handle;
#else
  GLFWwindow* m_GLFWWindow;
#endif
  ImVec2 m_WindowSize;
  bool m_CleanUpAfterRecording;
  Context m_Context;
  std::optional<float> m_UpdateUIScale;
  std::unique_ptr<MainWindow> mainWindow;
};

} // namespace gits::gui
