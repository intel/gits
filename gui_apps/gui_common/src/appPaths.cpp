// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "appPaths.h"

#include "log.h"

namespace {
// anonymous namespace for YAML keys to avoid typos and for easier refactoring
static constexpr const char* BASE_PATH_KEY = "BasePath";
static constexpr const char* CUSTOM_PLAYER_PATH_KEY = "CustomPlayerPath";
static constexpr const char* CAPTURE_CONFIG_PATH_KEY = "Capture.ConfigPath";
static constexpr const char* CAPTURE_TARGET_PATH_KEY = "Capture.CaptureTargetPath";
static constexpr const char* CAPTURE_OUTPUT_STREAM_PATH_KEY = "Capture.OutputStreamPath";
static constexpr const char* PLAYBACK_CONFIG_PATH_KEY = "Playback.ConfigPath";
static constexpr const char* PLAYBACK_INPUT_STREAM_PATH_KEY = "Playback.InputStreamPath";
static constexpr const char* PLAYBACK_ARTIFACTS_PATH_KEY = "Playback.ArtifactsPath";
static constexpr const char* PLAYBACK_TRACE_PATH_KEY = "Playback.TracePath";
static constexpr const char* SUBCAPTURE_CONFIG_PATH_KEY = "Subcapture.ConfigPath";
static constexpr const char* SUBCAPTURE_INPUT_STREAM_PATH_KEY = "Subcapture.InputStreamPath";
static constexpr const char* SUBCAPTURE_OUTPUT_STREAM_PATH_KEY = "Subcapture.OutputStreamPath";
static constexpr const char* STUDIO_ANALYSIS_PATH_KEY = "Studio.AnalysisPath";
} // namespace

namespace gits::gui {
void AppPaths::Read(const YAML::Node& yaml) {
  try {
    if (yaml[BASE_PATH_KEY]) {
      const auto basePath = std::filesystem::path(yaml[BASE_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(basePath)) {
        BasePath = basePath;
      }
    }
    if (yaml[CAPTURE_CONFIG_PATH_KEY]) {
      const auto captureConfigPath =
          std::filesystem::path(yaml[CAPTURE_CONFIG_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(captureConfigPath)) {
        Capture.ConfigPath = captureConfigPath;
      }
    }
    if (yaml[CAPTURE_TARGET_PATH_KEY]) {
      const auto captureTargetPath =
          std::filesystem::path(yaml[CAPTURE_TARGET_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(captureTargetPath)) {
        Capture.CaptureTargetPath = captureTargetPath;
      }
    }
    if (yaml[CAPTURE_OUTPUT_STREAM_PATH_KEY]) {
      const auto captureOutputStreamPath =
          std::filesystem::path(yaml[CAPTURE_OUTPUT_STREAM_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(captureOutputStreamPath)) {
        Capture.OutputStreamPath = captureOutputStreamPath;
      }
    }
    if (yaml[PLAYBACK_CONFIG_PATH_KEY]) {
      const auto playbackConfigPath =
          std::filesystem::path(yaml[PLAYBACK_CONFIG_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(playbackConfigPath)) {
        Playback.ConfigPath = playbackConfigPath;
      }
    }
    if (yaml[PLAYBACK_INPUT_STREAM_PATH_KEY]) {
      const auto playbackInputStreamPath =
          std::filesystem::path(yaml[PLAYBACK_INPUT_STREAM_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(playbackInputStreamPath)) {
        Playback.InputStreamPath = playbackInputStreamPath;
      }
    }
    if (yaml[PLAYBACK_ARTIFACTS_PATH_KEY]) {
      const auto playbackArtifactsPath =
          std::filesystem::path(yaml[PLAYBACK_ARTIFACTS_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(playbackArtifactsPath)) {
        Playback.ArtifactsPath = playbackArtifactsPath;
      }
    }
    if (yaml[PLAYBACK_TRACE_PATH_KEY]) {
      const auto playbackTracePath =
          std::filesystem::path(yaml[PLAYBACK_TRACE_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(playbackTracePath)) {
        Playback.TracePath = playbackTracePath;
      }
    }
    if (yaml[SUBCAPTURE_CONFIG_PATH_KEY]) {
      const auto subcaptureConfigPath =
          std::filesystem::path(yaml[SUBCAPTURE_CONFIG_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(subcaptureConfigPath)) {
        Subcapture.ConfigPath = subcaptureConfigPath;
      }
    }
    if (yaml[SUBCAPTURE_INPUT_STREAM_PATH_KEY]) {
      const auto subcaptureInputStreamPath =
          std::filesystem::path(yaml[SUBCAPTURE_INPUT_STREAM_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(subcaptureInputStreamPath)) {
        Subcapture.InputStreamPath = subcaptureInputStreamPath;
      }
    }
    if (yaml[SUBCAPTURE_OUTPUT_STREAM_PATH_KEY]) {
      const auto subcaptureOutputStreamPath =
          std::filesystem::path(yaml[SUBCAPTURE_OUTPUT_STREAM_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(subcaptureOutputStreamPath)) {
        Subcapture.OutputStreamPath = subcaptureOutputStreamPath;
      }
    }
    if (yaml[STUDIO_ANALYSIS_PATH_KEY]) {
      const auto studioAnalysisPath =
          std::filesystem::path(yaml[STUDIO_ANALYSIS_PATH_KEY].as<std::string>());
      if (std::filesystem::exists(studioAnalysisPath)) {
        Studio.AnalysisPath = studioAnalysisPath;
      }
    }
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "Couldn't read Launcher paths from file. Error: " << e.what();
  }
}

void AppPaths::Write(YAML::Emitter& out) const {
  out << YAML::Key << BASE_PATH_KEY << YAML::Value << BasePath.string();
  out << YAML::Key << CAPTURE_CONFIG_PATH_KEY << YAML::Value << Capture.ConfigPath.string();
  out << YAML::Key << CAPTURE_TARGET_PATH_KEY << YAML::Value << Capture.CaptureTargetPath.string();
  out << YAML::Key << CAPTURE_OUTPUT_STREAM_PATH_KEY << YAML::Value
      << Capture.OutputStreamPath.string();
  out << YAML::Key << PLAYBACK_CONFIG_PATH_KEY << YAML::Value << Playback.ConfigPath.string();
  out << YAML::Key << PLAYBACK_INPUT_STREAM_PATH_KEY << YAML::Value
      << Playback.InputStreamPath.string();
  out << YAML::Key << PLAYBACK_ARTIFACTS_PATH_KEY << YAML::Value << Playback.ArtifactsPath.string();
  out << YAML::Key << PLAYBACK_TRACE_PATH_KEY << YAML::Value << Playback.TracePath.string();
  out << YAML::Key << SUBCAPTURE_CONFIG_PATH_KEY << YAML::Value << Subcapture.ConfigPath.string();
  out << YAML::Key << SUBCAPTURE_INPUT_STREAM_PATH_KEY << YAML::Value
      << Subcapture.InputStreamPath.string();
  out << YAML::Key << SUBCAPTURE_OUTPUT_STREAM_PATH_KEY << YAML::Value
      << Subcapture.OutputStreamPath.string();
  out << YAML::Key << STUDIO_ANALYSIS_PATH_KEY << YAML::Value << Studio.AnalysisPath.string();
}

} // namespace gits::gui
