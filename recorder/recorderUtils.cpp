// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <windows.h>
#endif

namespace gits {

#ifdef GITS_PLATFORM_WINDOWS
BOOL CALLBACK CloseWindows(HWND hwnd, LPARAM lParam) {
  DWORD windowProcessId;
  GetWindowThreadProcessId(hwnd, &windowProcessId);
  if (windowProcessId == static_cast<DWORD>(lParam)) {
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return FALSE;
  }
  return TRUE;
}
#endif

bool ConfigureRecorder(const std::filesystem::path& configPath, bool legacyMode) {
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

  if (legacyMode) {
    // create file data and register it in GITS
    gits::CGits& inst = gits::CGits::Instance();
    std::unique_ptr<gits::CFile> file(new gits::CFile(inst.Version()));

    file->SetDiagnosticInfo(configPath);

    file->SetConfig(configPath);

    inst.RegisterFileRecorder(std::move(file));
  }

#ifdef GITS_PLATFORM_WINDOWS
  if (!legacyMode) {
    if (Configurator::Get().common.recorder.closeAppOnStopRecording) {
      MessageBus::get().subscribe(
          {PUBLISHER_RECORDER, TOPIC_END},
          [](Topic t, const MessagePtr& m) { EnumWindows(CloseWindows, GetCurrentProcessId()); });
    }
  }
#endif

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

  if (cfg.common.shared.waitForInput) {
    bool inputHandled = false;
#ifdef GITS_PLATFORM_WINDOWS
    // On Windows, show a message box instead of using the console
    int result = MessageBox(nullptr, "Waiting for input...", "GITS Recorder", MB_OK);
    inputHandled = (result != 0);
#endif
    if (!inputHandled) {
      // Always print to console
      std::cout << "Press ENTER to continue... (PID: " << pid << ")" << std::endl;
      std::cin.get();
    }
  }

  configured = true;
  return true;
}

} // namespace gits
