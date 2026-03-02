// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace gits::gui {

enum class Api {
  UNKNOWN = 0,
  DIRECTX,
  OPENGL,
  VULKAN,
  OPENCL,
  LEVELZERO,
  COUNT
};

enum class Mode {
  CAPTURE = 0,
  PLAYBACK,
  SUBCAPTURE,
  COUNT
};

enum class Path {
  // universal paths
  GITS_BASE = 0,
  CUSTOM_PLAYER,
  // unique paths - independent of mode
  SCREENSHOTS,
  TRACE,
  CAPTURE_TARGET,
  // shared paths across modes
  CONFIG,
  INPUT_STREAM,
  OUTPUT_STREAM,
  COUNT
};

struct FileDialogKeys {
  Path Path;
  Mode Mode;

  size_t GetValue() const {
    return static_cast<size_t>(Mode) * static_cast<size_t>(Path::COUNT) + static_cast<size_t>(Path);
  }

  const std::string ImGuiKey() const {
    return "FileDialog_" + std::to_string(static_cast<size_t>(Mode)) + "_" +
           std::to_string(static_cast<size_t>(Path));
  }
};

} // namespace gits::gui
