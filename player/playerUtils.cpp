// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   playerUtils.cpp
 *
 * @brief Utility functions for the gitsPlayer.
 *
 */

#include "playerUtils.h"

#include "configurationLib.h"
#include "argumentParser.h"
#include "tools.h"
#include "log.h"
#include "gits.h"

#include <string>
#include <filesystem>

namespace gits {
bool ends_with(const std::string& str, const std::string& ending) {
  if (str.size() < ending.size()) {
    return false;
  }

  return str.substr(str.size() - ending.size()) == ending;
}

bool ConfigurePlayer(const std::filesystem::path& playerPath, ArgumentParser& args) {
  static bool configured = false;
  if (configured) {
    return true;
  }

  Configurator::Instance().UpdateFromEnvironment();

  const std::filesystem::path configPath =
      args.ConfigFile ? std::filesystem::absolute(args.ConfigFile.Get())
                      : playerPath.parent_path() / Configurator::ConfigFileName();

  if (std::filesystem::exists(configPath)) {
    LOG_INFO << "Using configuration from: " << configPath;
    const auto result = Configurator::Instance().Load(configPath);
    if (!result) {
      LOG_ERROR << "Error reading in configuration from: " << configPath;
      return false;
    }
  } else {
    LOG_WARNING << "No configuration found at: " << configPath;
    if (args.ConfigFile) {
      LOG_ERROR << "Requested configuration file not found, terminating...";
      return false;
    } else {
      LOG_WARNING << "Built-in default configuration will be used";
    }
  }

  args.ArgumentConfig.UpdateConfiguration(&Configurator::GetMutable());

#ifdef GITS_PLATFORM_WINDOWS
  const auto& processName = gits::GetWindowsProcessName(_getpid());
#elif defined GITS_PLATFORM_LINUX
  const auto& processName = GetLinuxProcessName(getpid());
#endif
  Configurator::Instance().ApplyOverrides(configPath, processName);

  auto& cfg = Configurator::GetMutable();
  cfg.common.mode = GITSMode::MODE_PLAYER;
  cfg.common.player.applicationPath = playerPath;

  auto streamPath = std::filesystem::path();

  if (args.StreamPath) {
    streamPath = args.StreamPath.Get();
  } else if (!cfg.common.player.streamPath.empty()) {
    streamPath = cfg.common.player.streamPath;
  }

  if (!streamPath.empty()) {
    if (!std::filesystem::exists(streamPath)) {
      LOG_ERROR << "Specified stream path: '" << streamPath << "' does not exist";
      return false;
    }
    if (std::filesystem::is_directory(streamPath)) {
      LOG_INFO << "Specified stream path: '" << streamPath
               << "' is a directory, attempting to find a stream file.";
      std::vector<std::filesystem::path> matching_paths;
      for (const auto& dirEntry : std::filesystem::directory_iterator(streamPath)) {
        if (std::filesystem::is_directory(dirEntry)) {
          continue;
        }
        auto entryPathStr = dirEntry.path().string();
        if (ends_with(entryPathStr, ".gits2") || ends_with(entryPathStr, ".gits2.gz")) {
          matching_paths.push_back(dirEntry.path());
        }
      }

      if (matching_paths.size() != 1) {
        if (matching_paths.empty()) {
          LOG_ERROR << "Specified directory does not contain a stream file.";
        }
        if (matching_paths.size() > 1) {
          LOG_ERROR
              << "Too many stream files in specified directory. Specify stream file explicitly.";
        }
        LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
        return false;
      }
      streamPath = matching_paths[0];
      LOG_INFO << "Using stream file '" << streamPath;
    }

    streamPath = std::filesystem::absolute(streamPath);
    cfg.common.player.streamPath = streamPath;
    cfg.common.player.streamDir = streamPath.parent_path();
  }

  if (cfg.common.player.streamPath.empty()) {
    LOG_ERROR << "No stream was passed to gitsPlayer.";
    return false;
  }

  Configurator::Instance().DeriveData();

  // TODO: Config - This should store current config, not only the file
#if defined(WITH_DIRECTX) && defined(GITS_PLATFORM_WINDOWS)
  if ((Configurator::Get().common.mode == GITSMode::MODE_PLAYER) &&
      (Configurator::Get().directx.features.subcapture.enabled)) {
#else
  if (Configurator::Get().common.mode == GITSMode::MODE_RECORDER) {
#endif
    // create file data and register it in GITS
    gits::CGits& inst = gits::CGits::Instance();
    std::unique_ptr<gits::CFile> file(new gits::CFile(inst.Version()));

    file->SetDiagnosticInfo(configPath);

    file->SetConfig(std::move(configPath));

    inst.RegisterFileRecorder(std::move(file));
  }

#ifdef GITS_PLATFORM_WINDOWS
  Configurator::PrepareSubcapturePath();
#endif

  return true;
}
} // namespace gits
