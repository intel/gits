// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace gits::gui {
struct AppPaths {
  std::filesystem::path BasePath;

  struct CapturePaths {
    std::filesystem::path ConfigPath;
    std::filesystem::path CaptureTargetPath;
    std::filesystem::path OutputStreamPath;
  } Capture;

  struct PlaybackPaths {
    std::filesystem::path ConfigPath;
    std::filesystem::path InputStreamPath;
    std::filesystem::path ArtifactsPath;
    std::filesystem::path TracePath;
  } Playback;

  struct SubcapturePaths {
    std::filesystem::path ConfigPath;
    std::filesystem::path InputStreamPath;
    std::filesystem::path OutputStreamPath;
  } Subcapture;

  struct Studio {
    std::filesystem::path AnalysisPath;
  } Studio;

  void Read(const YAML::Node& yaml);
  void Write(YAML::Emitter& out) const;
};
} // namespace gits::gui
