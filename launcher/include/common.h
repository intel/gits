// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <filesystem>

namespace gits::gui {

namespace filesystem_names {
static constexpr const char* RECORDER_CONFIG_FILENAME = "gits_config.yml";
static constexpr const char* RECORDER_DIRECTORY_NAME = "Recorder";
static constexpr const char* PLAYER_CONFIG_FILENAME = "gits_config.yml";
static constexpr const char* PLAYER_TEMPORARY_CONFIG_FILENAME =
    "gits_player_temporary_config_from_launcher.yml"; // For when we serialize the in-memory configuration into a temporary config file
static constexpr const char* GITS_PLAYER_WIN = "gitsPlayer.exe";
static constexpr const char* GITS_PLAYER_LINUX = "gitsPlayer";
static constexpr const char* GITS_RECORDER_WIN = "gitsRecorder.dll";
static constexpr const char* GITS_RECORDER_LINUX = "libgitsRecorder.so";
static constexpr const char* GITS_STREAM = "stream.gits2";
} // namespace filesystem_names

namespace yaml_constants {
static constexpr const char* OVERRIDES_KEY = "Overrides";
}

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
  SUBCAPTURE
};

enum class Path {
  // universal paths
  GITS_BASE = 0,
  // unique paths - independent of mode
  SCREENSHOTS,
  CAPTURE_TARGET,
  // shared paths across modes
  CONFIG,
  INPUT_STREAM,
  OUTPUT_STREAM,
  TRACE,
  // Custom
  GITS_LOG,
  CONFIG_EXPORT
};

struct FileDialogKey {
  Path Path;
  Mode Mode;

  const std::string ImGuiKey() const {
    return "FileDialog_" + std::to_string(static_cast<size_t>(Mode)) + "_" +
           std::to_string(static_cast<size_t>(Path));
  }
};

struct TemporaryConfigInfo {
  std::filesystem::path TemporaryConfigPath = "";
  bool OriginalNeedsRestoring = false;
  std::filesystem::path OriginalBackupPath = "";
};

} // namespace gits::gui
