// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace gits {
namespace windowing {

enum class WindowEvent {
  Close,
  Stop,
  TogglePause,
  ToggleInteractive
};

class PlayerWindow {
public:
  virtual ~PlayerWindow() = default;

  virtual std::vector<WindowEvent> Poll() = 0;
  virtual void Resize(uint32_t width, uint32_t height) = 0;
  virtual void SetPosition(int32_t x, int32_t y) = 0;
  virtual void SetVisibility(bool visible) = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::pair<uint64_t, uint64_t> NativeHandles() const = 0;
};

} // namespace windowing
} // namespace gits
