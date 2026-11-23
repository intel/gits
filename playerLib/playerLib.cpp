// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLib.h"
#include "log.h"
#include "exception.h"
#include "exceptionHandler.h"
#include "argumentParser.h"
#include "streamPlayer.h"
#include "gits.h"
#include "streamHeader.h"
#include "playerUtils.h"
#if defined(WITH_DIRECTX) || defined(WITH_VULKAN)
#include "imGuiHUD.h"
#endif

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <iomanip>

namespace gits {

void CheckSystemMemoryCompatibility() {
#ifdef GITS_PLATFORM_WINDOWS
  // Read the total physical memory recorded during capture from stream metadata.
  // This value comes from WMI Win32_ComputerSystem and is stored as a string.
  std::string property("diag.os_specific.Win32_ComputerSystem.TotalPhysicalMemory");
  auto capturedMemoryOpt = stream::StreamHeader::Get().FindProperty(property);
  if (!capturedMemoryOpt) {
    LOG_TRACE << "Stream metadata does not contain capture machine memory info.";
    return;
  }

  uint64_t capturedMemoryBytes = 0;
  try {
    std::string capturedMemoryStr = capturedMemoryOpt->get<std::string>();
    capturedMemoryBytes = std::stoull(capturedMemoryStr);
  } catch (const std::exception&) {
    LOG_TRACE << "Could not parse capture machine memory from stream metadata.";
    return;
  }

  // Query the current (replay) machine's physical memory.
  MEMORYSTATUSEX memStatus{};
  memStatus.dwLength = sizeof(memStatus);
  if (!GlobalMemoryStatusEx(&memStatus)) {
    LOG_TRACE << "Failed to query replay machine memory info.";
    return;
  }

  uint64_t replayMemoryBytes = memStatus.ullTotalPhys;

  auto toGB = [](uint64_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
  };

  LOG_INFO << "Capture machine RAM: " << std::fixed << std::setprecision(1)
           << toGB(capturedMemoryBytes) << " GB, Replay machine RAM: " << toGB(replayMemoryBytes)
           << " GB";

  if (replayMemoryBytes < capturedMemoryBytes) {
    LOG_WARNING << "The replay machine has less physical memory (" << std::fixed
                << std::setprecision(1) << toGB(replayMemoryBytes)
                << " GB) than the capture machine (" << toGB(capturedMemoryBytes)
                << " GB). This may cause out-of-memory errors or degraded performance.";
  }
#endif
}

bool g_InitializedForPlay{};
std::set<std::string> g_ArgsFilterTags;
std::string g_ApplicationName;
std::optional<nlohmann::ordered_json> g_GpuPreferenceExports;

bool ArgsFilterTagsFunc(const args::Base& item) {
  if (g_ArgsFilterTags.size() <= 0) {
    return true;
  }

  // The lambdas are used to make the filtering case insensitive
  auto toUpper = [](const std::string& str) {
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    return upperStr;
  };

  for (const auto& entry : item.GetTags()) {
    auto comparisonFunc = [&toUpper, &entry](const std::string& tag) {
      return toUpper(tag) == toUpper(entry);
    };

    if (std::find_if(g_ArgsFilterTags.begin(), g_ArgsFilterTags.end(), comparisonFunc) !=
        g_ArgsFilterTags.end()) {
      return true;
    }
  }
  return false;
}

int Initialize(int argc, char* argv[]) {
  log::Initialize(gits::LogLevel::INFO);
  log::AddConsoleAppender(); // Will be removed after config parsing if disabled in config.

  std::filesystem::path playerPath = "";
  auto argsVector = std::vector<std::string>(argv, argv + argc);
  if (argsVector.size() >= 1) {
    playerPath = argsVector[0];
    argsVector.erase(argsVector.begin());
  }
  auto args = ArgumentParser(argsVector);

  args::HideGroupSection = true;
  args::HiddenOptionSuffixMarker = '!';

  args.Parser.helpParams.helpindent = 30;
  args.Parser.helpParams.eachgroupindent = 0;
  args.Parser.helpParams.programName = "gitsPlayer";
  args.Parser.helpParams.addNewlineBeforeDescription = true;
  args.Parser.helpParams.showCommandFullHelp = true;

  switch (args.ParsingResult) {
  case ParsingSyntaxError:
    LOG_ERROR << "Error during command line parsing:\n" << args.Output.str();
    LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
    return EXIT_FAILURE;
  case ParsingSemanticError:
    LOG_ERROR << "Error during command line parsing:\n" << args.Output.str();
    return EXIT_FAILURE;
  default:
    break;
  }

  if ((args.ParsingResult == ShowHelp) || (args.HelpMenu)) {
    if (args.HelpMenu) {
      g_ArgsFilterTags.insert(args.HelpMenu.Get());
      args::GlobalFilterOption = ArgsFilterTagsFunc;
    }
    std::cout
        << std::endl
        << std::endl
        << args.Parser.Help() << std::endl
        << "All options of a configfile can be set via commandline by using the keypath and value."
        << std::endl;
    return EXIT_SUCCESS;
  }

  if (args.Version) {
    // Print version and quit.
    CGits& inst = CGits::Instance();
    std::cout << inst << std::endl;
    return EXIT_SUCCESS;
  }

  try {
    if (!ConfigurePlayer(playerPath, args)) {
      LOG_ERROR << "Encountered error while configuring player";
      LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
      return EXIT_FAILURE;
    }
  } catch (const std::exception& e) {
    LOG_ERROR << "Encountered error while configuring player:\n" << e.what();
    LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
    return EXIT_FAILURE;
  }

  // Input (arguments, environment and configuration) validated
  // Quit if --validate is used
  if (args.Validate) {
    LOG_INFO << "Used \"--validate\". Will not start the playback session.";
    return EXIT_SUCCESS;
  }

  // Print version.
  CGits& inst = CGits::Instance();
  LOG_INFO << inst;

  Configurator::Instance().LogChangedFields();

  const auto& cfg = Configurator::Get();
  log::SetMaxSeverity(cfg.common.shared.thresholdLogLevel);
  if (!cfg.common.shared.logToConsole.value_or(true)) {
    log::RemoveConsoleAppender();
  }
  if (!cfg.common.player.outputTracePath.empty()) {
    log::AddFileAppender(cfg.common.player.outputTracePath);
  }

  if (cfg.common.shared.waitForInput) {
    // Always print to console
    std::cout << "Press ENTER to continue..." << std::endl;
    std::cin.get();
  }

#if defined(WITH_DIRECTX) || defined(WITH_VULKAN)
  auto pImGuiHUD = std::make_unique<ImGuiHUD>();
  CGits::Instance().SetImGuiHUD(std::move(pImGuiHUD));
#endif

#if defined GITS_PLATFORM_WINDOWS
  if (cfg.common.player.escalatePriority) {
    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
      LOG_INFO << "Escalated process priority to realtime priority";
    } else {
      LOG_WARNING << "Priority escalation failed";
    }
  }
#endif

  // initialize GITS
  LOG_INFO << "Initializing...";

  LOG_INFO << "Loading...";
  if (!IsSupportedStream(cfg.common.player.streamPath)) {
    CGits::Instance().Dispose();
    return EXIT_FAILURE;
  }
  LogStreamRecordingVersion();

  // Compare capture vs replay machine RAM and warn if replay has less.
  CheckSystemMemoryCompatibility();

  if (cfg.common.player.executableNameOverride.enabled) {
    if (!cfg.common.player.executableNameOverride.customName.empty()) {
      g_ApplicationName = cfg.common.player.executableNameOverride.customName;
    } else {
      g_ApplicationName = stream::StreamHeader::Get().GetApplicationName();
    }
    if (g_ApplicationName.empty()) {
      LOG_WARNING << "Couldn't obtain the original application name, Player will not be renamed.";
    }
  }

  const char* gpuPreferenceExports = "diag.app.gpu_preference_exports";
  g_GpuPreferenceExports = stream::StreamHeader::Get().FindProperty(gpuPreferenceExports);

  g_InitializedForPlay = true;
  return EXIT_SUCCESS;
}

int Play() {
  auto cleanup = []() {
    CGits::Instance().Dispose();
    g_InitializedForPlay = false;
  };

  int returnValue = EXIT_SUCCESS;
  if (!g_InitializedForPlay) {
    cleanup();
    return returnValue;
  }

  try {
    const auto& cfg = Configurator::Get();

#if defined GITS_PLATFORM_WINDOWS
    int previousDesktopWidth = GetSystemMetrics(SM_CXSCREEN);
    int previousDesktopHeight = GetSystemMetrics(SM_CYSCREEN);
    if (cfg.common.player.forceDesktopResolution.enabled) {
      DEVMODE devmode;
      devmode.dmPelsWidth = cfg.common.player.forceDesktopResolution.width;
      devmode.dmPelsHeight = cfg.common.player.forceDesktopResolution.height;
      devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmSize = sizeof(DEVMODE);

      ChangeDisplaySettings(&devmode, 0);
    }
#endif

    if (cfg.common.player.diags) {
      std::cout << stream::StreamHeader::Get().GetPropertiesDump();
      cleanup();
      return EXIT_SUCCESS;
    }

#ifdef GITS_PLATFORM_WINDOWS
    auto pid = _getpid();
    auto processName = gits::GetWindowsProcessName(pid);
#elif defined GITS_PLATFORM_LINUX
    auto pid = getpid();
    auto processName = GetLinuxProcessName(pid);
#endif
    processName = processName.empty() ? "<unknown>" : processName;
#if defined(WITH_DIRECTX) || defined(WITH_VULKAN)
    CGits::Instance().GetImGuiHUD()->SetApplicationInfo(processName, pid);
#endif

    // check if all functions can be run on that system
    LOG_INFO << "Playing...";
    CGits::Instance().GetMessageBus().publish({PUBLISHER_PLAYER, TOPIC_PROGRAM_START},
                                              std::make_shared<ProgramMessage>());

    PlayStream(cfg.common.player.streamPath);

#ifdef GITS_PLATFORM_WINDOWS
    if (cfg.common.player.forceDesktopResolution.enabled) {
      DEVMODE devmode;
      devmode.dmPelsWidth = previousDesktopWidth;
      devmode.dmPelsHeight = previousDesktopHeight;
      devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmSize = sizeof(DEVMODE);

      ChangeDisplaySettings(&devmode, 0);
    }
#endif

    LOG_INFO << "Finishing...";
  } catch (Exception& ex) {
    LOG_ERROR << ex.what();
    returnValue = EXIT_FAILURE;
  } catch (std::exception& ex) {
    LOG_ERROR << ex.what();
    returnValue = EXIT_FAILURE;
  } catch (...) {
    LOG_ERROR << "Unrecognized exception was raised during GITS execution!!!";
    returnValue = EXIT_FAILURE;
  }

  CGits::Instance().GetMessageBus().publish({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                            std::make_shared<ProgramMessage>());

  cleanup();
  return returnValue;
}

} // namespace gits

int STDCALL Initialize(int argc, char* argv[]) {
  return gits::RunNoThrow("Initialize", gits::Initialize, argc, argv);
}

const char* STDCALL GetApplicationName() {
  return gits::g_ApplicationName.c_str();
}

uint32_t STDCALL GetGpuPreferenceExport(const char* name) {
  const auto& exports = gits::g_GpuPreferenceExports;
  if (!exports || !exports->is_object()) {
    return 0;
  }
  const auto value = exports->find(name);
  if (value == exports->end() || !value->is_number_unsigned()) {
    return 0;
  }
  return value->get<uint32_t>();
}

int STDCALL Play() {
  return gits::RunNoThrow("Play", gits::Play);
}
