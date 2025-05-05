#include "playerUtils.h"

#include <string>
#include <filesystem>

#include "configurationLib.h"
#include "argumentParser.h"
#include "tools.h"
#include "log.h"
#include "gits.h"

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

  std::filesystem::path configPath =
      args.ConfigFile ? std::filesystem::absolute(args.ConfigFile.Get())
                      : playerPath.parent_path() / Configurator::ConfigFileName();

  if (std::filesystem::exists(configPath)) {
    Log(INFO) << "Using configuration from: " << configPath;
    auto result = Configurator::Instance().Load(configPath);
    if (!result) {
      Log(ERR) << "Error reading in configuration from: " << configPath;
      return false;
    }
  } else {
    Log(WARN) << "No configuration found at: " << configPath;
    if (args.ConfigFile) {
      Log(ERR) << "Requested configuration file not found, terminating...";
      return false;
    } else {
      Log(WARN) << "Built-in default configuration will be used";
    }
  }

  args.ArgumentConfig.UpdateConfiguration(&Configurator::GetMutable());

#ifdef GITS_PLATFORM_WINDOWS
  auto processName = gits::GetWindowsProcessName(_getpid());
#elif defined GITS_PLATFORM_LINUX
  auto processName = GetLinuxProcessName(getpid());
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
      Log(ERR) << "Specified stream path: '" << streamPath << "' does not exist";
      return false;
    }
    if (std::filesystem::is_directory(streamPath)) {
      Log(INFO) << "Specified stream path: '" << streamPath
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

      if (matching_paths.empty()) {
        throw std::runtime_error("Specified directory does not contain a stream file.");
      }
      if (matching_paths.size() > 1) {
        throw std::runtime_error(
            "Too many stream files in specified directory. Specify stream file explicitly.");
      }
      streamPath = matching_paths[0];
      Log(INFO) << "Using stream file '" << streamPath << "'\n";
    }

    cfg.common.player.streamPath = streamPath;
    cfg.common.player.streamDir = streamPath.parent_path();
  }

  if (cfg.common.player.streamPath.empty()) {
    Log(ERR) << "No stream was passed to gitsPlayer.";
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

    file->SetConfig(configPath);

    inst.RegisterFileRecorder(std::move(file));
  }

#ifdef GITS_PLATFORM_WINDOWS
  Configurator::PrepareSubcapturePath();
#endif

  return true;
}
} // namespace gits
