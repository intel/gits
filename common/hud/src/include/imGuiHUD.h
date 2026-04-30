// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <variant>

#include "imgui.h"
#include "enumsAuto.h"

namespace gits {

struct ImGuiWidget_Text {
  std::string text;
};

struct ImGuiWidget_Separator {};

using ImGuiWidget = std::variant<ImGuiWidget_Text, ImGuiWidget_Separator>;

class IHUDPlugin {
public:
  virtual ~IHUDPlugin() = default;
  virtual const std::vector<ImGuiWidget>* HUDCallback() = 0;
};

class ImGuiHUD {
public:
  ImGuiHUD();
  ~ImGuiHUD();

  int AddHUDPlugin(IHUDPlugin* plugin, bool flag);
  void RemoveHUDPlugin(int pluginID);
  void SetApplicationInfo(const std::string& name, int pid);
  void SetBackBufferInfo(uint64_t width, uint64_t height, size_t count = 1);
  void Render();

  void SetupImGUI(float dpi_scale);

private:
  ImVec2 GetWindowSize(float uiScale);
  ImVec2 ComputeHUDSizeHint(float uiScale);
  void PositionHUD(gits::HUDAnchor anchor, const ImVec2& windowSize, const ImVec2& padding);
  void ExecuteCallbacks();
  double CalculateFPS(std::chrono::high_resolution_clock::time_point now);

  int _nextPluginID = 0;
  std::map<int, IHUDPlugin*> _plugins;
  mutable std::shared_mutex _pluginMutex;
  bool _hasExternalCallbacks = false;
  int _tableRows = 0;
  std::vector<std::array<std::string, 2>> _dataTable;
  std::string _strVersion = "<unknown>";
  ImVec2 _backBufferSize = ImVec2(0.0f, 0.0f);
  size_t _backBufferCount = 0;
  bool _backBufferSet = false;
  std::string _applicationName = "<unknown>";
  int _applicationPid = -1;

  // Render-to-Render frame time samples for FPS calculation, stored as pairs of (timestamp, frame time in seconds)
  std::deque<std::pair<std::chrono::high_resolution_clock::time_point, double>> _frameTimes;
};

} // namespace gits
