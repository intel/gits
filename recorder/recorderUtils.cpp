// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderUtils.cpp
 *
 * @brief Utility functions for the gitsRecorder.
 */

#include "recorderUtils.h"

#include <filesystem>

#include "configurationLib.h"
#include "log.h"
#include "gits.h"
#include "diagnostic.h"

#if defined GITS_PLATFORM_WINDOWS && (WITH_DIRECTX || WITH_VULKAN)
#include "imGuiHUD.h"
#endif

namespace gits {
bool ConfigureRecorder(const std::filesystem::path& configPath) {
  static bool configured = false;
  if (configured) {
    return true;
  }

  // Initialize the logger first with the default logLevel
  const auto& cfg = Configurator::Get();
  log::Initialize(cfg.common.shared.thresholdLogLevel);
  log::AddFileAppender(configPath.parent_path());
  log::AddConsoleAppender(); // Will be removed after config parsing if disabled in config.

  Configurator::Instance().UpdateFromEnvironment();

  const auto result = Configurator::Instance().Load(configPath);
  if (!result) {
    return false;
  }

#ifdef GITS_PLATFORM_WINDOWS
  const auto pid = _getpid();
  const auto& processName = gits::GetWindowsProcessName(pid);
#elif defined GITS_PLATFORM_LINUX
  const auto pid = getpid();
  const auto& processName = GetLinuxProcessName(pid);
#endif
  const auto& processNameHUD = processName.empty() ? "<unknown>" : processName;
  Configurator::Instance().ApplyOverrides(configPath, processName);

  Configurator::GetMutable().common.mode = GITSMode::MODE_RECORDER;

  Configurator::Instance().DeriveData();

  // TODO: Config - This should store current config, not only the file

  // create file data and register it in GITS
  gits::CGits& inst = gits::CGits::Instance();
  std::unique_ptr<gits::CFile> file(new gits::CFile(inst.Version()));

  file->SetDiagnosticInfo(configPath);

  file->SetConfig(configPath);

  inst.RegisterFileRecorder(std::move(file));

#if defined GITS_PLATFORM_WINDOWS && (WITH_DIRECTX || WITH_VULKAN)
  auto pImGuiHUD = std::make_unique<ImGuiHUD>();
  CGits::Instance().SetImGuiHUD(std::move(pImGuiHUD));
  CGits::Instance().GetImGuiHUD()->SetApplicationInfo(processNameHUD, pid);
#endif

  Configurator::Instance().LogChangedFields();

  // Update the log parameters after loading in the config
  log::SetMaxSeverity(cfg.common.shared.thresholdLogLevel);
  if (!cfg.common.shared.logToConsole.value_or(false)) {
    log::RemoveConsoleAppender();
  }

  LOG_INFO << "GITS configured for process: " << processNameHUD;

  configured = true;
  return true;
}
} // namespace gits
