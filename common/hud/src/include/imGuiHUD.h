// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>

#include "imgui.h"
#include "enumsAuto.h"

namespace gits {
using RenderImGuiFunc = std::function<void()>;

class ImGuiHUD {
public:
  ImGuiHUD();

  void AddCallback(RenderImGuiFunc callback);
  void SetApplicationInfo(const std::string& name, int pid);
  void SetBackBufferInfo(uint64_t width, uint64_t height, size_t count = 1);
  void Render();

  void SetupImGUI(float dpi_scale);

private:
  void PositionHUD(gits::HUDAnchor anchor, const ImVec2& windowSize, const ImVec2& padding);

  void ExecuteCallbacks();

  std::vector<RenderImGuiFunc> _callbacks;
  std::mutex _callbackMutex;
  bool _hasExternalCallbacks = false;
  int _tableRows = 0;
  std::vector<std::array<std::string, 2>> _dataTable;
  std::string _strVersion = "<unknown>";
  ImVec2 _backBufferSize = ImVec2(0.0f, 0.0f);
  size_t _backBufferCount = 0;
  bool _backBufferSet = false;
  std::string _applicationName = "<unknown>";
  int _applicationPid = -1;

  // Frame timing for FPS measurement
  std::chrono::high_resolution_clock::time_point _lastFrameTime;
  double _frameDuration = 0.0;
};

} // namespace gits
