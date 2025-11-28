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

#include <windows.h>

#include "fileActions.h"
#include "imGuiHelper.h"

#include "context.h"
#include "launcherConfig.h"

namespace gits::gui {
struct Settings {
  static constexpr const wchar_t* WINDOW_TITLE = L"GITS Launcher";

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
  GUIController(HWND hwnd);

  void SetupGui();
  void DrawGui();
  void TeardownGui();
  void DestroyGui();
  void Resized();
  void Positioned();

private:
  void RenderUI();

private:
  HWND m_Handle;
  ImVec2 m_WindowSize;
  bool m_CleanUpAfterRecording;
  Context m_Context;
  LauncherConfig m_LauncherConfig;
};

} // namespace gits::gui
