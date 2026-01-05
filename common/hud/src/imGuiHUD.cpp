// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiHUD.h"

#include <algorithm>

#include "gits.h"
#include "enumsAuto.h"
#include "configurator.h"
#include "log.h"
#include "imGuiHelper.h"

namespace gits {

namespace {
struct Settings {
  static constexpr auto HUD_SCALE = 1.15f;
  static constexpr auto HUD_PADDING = ImVec2(8, 8);
  static constexpr auto HUD_WINDOW_STYLE = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_AlwaysAutoResize;
};

} // namespace

ImGuiHUD::ImGuiHUD() {
  std::ostringstream oss;
  oss << CGits::Instance().Version();
  _strVersion = oss.str();
  _lastFrameTime = std::chrono::high_resolution_clock::now();
}

void ImGuiHUD::AddCallback(RenderImGuiFunc callback) {
  std::lock_guard<std::mutex> lock(_callbackMutex);
  _callbacks.push_back(callback);

  auto& cfgHud = Configurator::Get().common.shared.hud;
  _hasExternalCallbacks = !_callbacks.empty();
}

void ImGuiHUD::SetApplicationInfo(const std::string& name, int pid) {
  _applicationName = name;
  _applicationPid = pid;
}

void ImGuiHUD::ExecuteCallbacks() {
  std::lock_guard<std::mutex> lock(_callbackMutex);
  for (const auto& callback : _callbacks) {
    callback();
  }
}

void ImGuiHUD::PositionHUD(gits::HUDAnchor anchor,
                           const ImVec2& windowSize,
                           const ImVec2& padding) {
  const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
  float x = 0;
  if (anchor == gits::HUDAnchor::TOP_LEFT || anchor == gits::HUDAnchor::CENTER_LEFT ||
      anchor == gits::HUDAnchor::BOTTOM_LEFT) {
    x = mainViewport->WorkPos.x + padding.x;
  } else if (anchor == gits::HUDAnchor::TOP_CENTER || anchor == gits::HUDAnchor::CENTER ||
             anchor == gits::HUDAnchor::BOTTOM_CENTER) {
    x = mainViewport->WorkPos.x + (mainViewport->Size.x - windowSize.x) / 2.0f;
  } else { // <X>_RIGHT
    x = mainViewport->WorkPos.x + mainViewport->Size.x - windowSize.x - padding.x;
  }

  float y = 0;
  if (anchor == gits::HUDAnchor::BOTTOM_LEFT || anchor == gits::HUDAnchor::BOTTOM_CENTER ||
      anchor == gits::HUDAnchor::BOTTOM_RIGHT) {
    y = mainViewport->WorkPos.y + mainViewport->Size.y - windowSize.y - padding.y;
  } else if (anchor == gits::HUDAnchor::CENTER_LEFT || anchor == gits::HUDAnchor::CENTER ||
             anchor == gits::HUDAnchor::CENTER_RIGHT) {
    y = mainViewport->WorkPos.y + (mainViewport->Size.y - windowSize.y) / 2.0f;
  } else { // TOP_<X>
    y = mainViewport->WorkPos.y + padding.y;
  }
  ImGui::SetWindowPos(ImVec2(x, y), ImGuiCond_Always);
}

void ImGuiHUD::SetBackBufferInfo(uint64_t width, uint64_t height, size_t count) {
  if (width > 0 && height > 0) {
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(width, height);
  }

  _backBufferSize = ImVec2(static_cast<float>(width), static_cast<float>(height));
  _backBufferCount = count;
  _backBufferSet = true;

  // Print a warning if the width or height changed after initialization
  static uint64_t initialWidth = width;
  static uint64_t initialHeight = height;
  static bool printWarning = false;
  if (!printWarning && (initialWidth != width || initialHeight != height)) {
    LOG_WARNING << "ImGui HUD: BackBuffer resized - HUD can appear 'stretched'.";
    printWarning = true;
  }
}

void ImGuiHUD::Render() {
  auto now = std::chrono::high_resolution_clock::now();
  _frameDuration = std::chrono::duration<double>(now - _lastFrameTime).count();
  _lastFrameTime = now;

  if ((_tableRows == 0) && !_hasExternalCallbacks) {
    return;
  }

  auto& cfgHud = Configurator::Get().common.shared.hud;

  const auto winSizeMin = ImVec2(2.0f, 2.0f);
  const auto winSizeMax = ImVec2(FLT_MAX, FLT_MAX);
  ImGui::SetNextWindowSizeConstraints(winSizeMin, winSizeMax);

  if (ImGui::Begin("GITS HUD", nullptr, Settings::HUD_WINDOW_STYLE)) {
    if (_tableRows > 0) {
      uInt idx = 0;
      if (cfgHud.fields.applicationInfo) {
        _dataTable[idx][0] = "Application";
        _dataTable[idx][1] = _applicationName + " (" + std::to_string(_applicationPid) + ")";
        ++idx;
      }
      if (cfgHud.fields.version) {
        _dataTable[idx][0] = "Version";
        _dataTable[idx][1] = _strVersion;
        ++idx;
      }
      if (_backBufferSet && cfgHud.fields.backBufferResolution) {
        _dataTable[idx][0] = "BackBuffer, Cnt";
        _dataTable[idx][1] =
            ImGuiHelper::ToStr(_backBufferSize) + ", " + std::to_string(_backBufferCount);
        ++idx;
      }
      if (cfgHud.fields.frameNumber) {
        _dataTable[idx][0] = "Frame";
        _dataTable[idx][1] = std::to_string(CGits::Instance().CurrentFrame());
        ++idx;
      }
      if (cfgHud.fields.fps) {
        double fps = (_frameDuration > 0.0) ? (1.0 / _frameDuration) : 0.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << std::setw(5) << fps;
        _dataTable[idx][0] = "FPS";
        _dataTable[idx][1] = oss.str();
        ++idx;
      }

      if (ImGui::BeginTable("HUD-Table", 2)) {
        ImGuiHelper::RenderTableContent(_dataTable);
        ImGui::EndTable();
      }
    }

    if (_hasExternalCallbacks) {
      if (_tableRows > 0) {
        ImGui::Separator();
      }
      ExecuteCallbacks();
    }

    const ImVec2 windowSize = GetWindowSize(cfgHud.uiScale);
    PositionHUD(cfgHud.position, windowSize, Settings::HUD_PADDING);
  }
  ImGui::End();
}

ImVec2 ImGuiHUD::GetWindowSize(float uiScale) {
  static bool initialRun{true};
  if (!initialRun) {
    return ImGui::GetWindowSize();
  }
  initialRun = false;
  return ComputeHUDSizeHint(uiScale);
}

ImVec2 ImGuiHUD::ComputeHUDSizeHint(float uiScale) {
  ImVec2 winSizeHint{0, 0};

  if (_tableRows > 0) {
    float contentHeight = 0.f;
    float contentLength = 0.f;
    auto preComputeContentSize = [this, &contentHeight, &contentLength]() {
      float const heightCoefficient = 18.f; // from experiment
      float const lengthCoefficient = 6.8f; // from experiment
      constexpr unsigned margin = 17;       // from experiment

      uInt maxLengths[2]{0, 0};
      auto collectMax = [this, &maxLengths](uInt idx) {
        for (auto i = 0u; i < sizeof(maxLengths) / sizeof(maxLengths[0]); ++i) {
          maxLengths[i] = std::max<size_t>(maxLengths[i], _dataTable[idx][i].size());
        }
      };

      for (auto i = 0u; i < _dataTable.size(); ++i) {
        collectMax(i);
      }
      contentHeight = heightCoefficient * _dataTable.size() + margin;
      contentLength = lengthCoefficient * (maxLengths[0] + maxLengths[1]) + margin;
    };

    preComputeContentSize();
    winSizeHint.x = contentLength * uiScale;
    winSizeHint.y = contentHeight * uiScale;
  }

  return winSizeHint;
}

void ImGuiHUD::SetupImGUI(float dpi_scale) {
  auto& cfgHud = Configurator::Get().common.shared.hud;

  _tableRows = static_cast<int>(cfgHud.fields.applicationInfo) +
               static_cast<int>(cfgHud.fields.version) +
               static_cast<int>(cfgHud.fields.backBufferResolution) +
               static_cast<int>(cfgHud.fields.frameNumber) + static_cast<int>(cfgHud.fields.fps);

  std::generate_n(std::back_inserter(_dataTable), _tableRows, []() {
    return std::array<std::string, 2>{"", ""};
  });

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // no interaction for now.
  io.ConfigFlags |=
      ImGuiConfigFlags_NoKeyboard | ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
  io.ConfigNavCaptureKeyboard = false;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  auto& cfg = Configurator::Get();
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, cfgHud.opacity);

  ImGuiHelper::UpdateUIScaling(dpi_scale * cfgHud.uiScale * Settings::HUD_SCALE);
  _backBufferSize = ImGui::GetMainViewport()->Size;
}

}; // namespace gits
