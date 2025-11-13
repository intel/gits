// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   playerMain.cpp
 *
 * @brief main() function of gitsPlayer.
 *
 */

#include "argumentParser.h"

#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include "StackWalker.h"
#include <psapi.h>
#ifdef WITH_VULKAN
#include "vulkanRenderDocUtil.h"
#endif
#endif

#include "gits.h"
#include "openglLibrary.h"
#ifdef WITH_OPENCL
#include "openclLibrary.h"
#endif
#ifdef WITH_VULKAN
#include "vulkanLibrary.h"
#endif
#if defined WITH_LEVELZERO
#include "l0Library.h"
#endif
#if defined WITH_OCLOC
#include "oclocLibrary.h"
#endif
#if defined WITH_DIRECTX
#include "directXLibrary.h"
#endif

#include "player.h"
#include "exception.h"
#include "log.h"
#include "performance.h"
#include "windowing.h"
#include "timer.h"
#include "runner.h"
#include "sequentialExecutor.h"
#include "pragmas.h"
#include "playerOptions.h"
#include "message_pump.h"
#if defined GITS_PLATFORM_WINDOWS
#include "recorder.h"
#endif
#include "configurationLib.h"
#include "diagnostic.h"
#include "playerUtils.h"
#include "log.h"

#if defined GITS_PLATFORM_WINDOWS && (WITH_DIRECTX || WITH_VULKAN)
#include "imGuiHUD.h"
#endif

#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <filesystem>

namespace gits {
// Gits player message loop
//    - play gits, when state is RUNNING
//    - stop processing messages if state is FINISHED
//    - pass any keypresses to player handles
class GitsMessagePump : public MessagePump {
public:
  GitsMessagePump(CPlayer& player) : MessagePump(), player_(player) {}

protected:
  void idle() {
    if (player_.State() == CPlayer::STATE_RUNNING) {
      player_.Play();
    }
    if (player_.State() == CPlayer::STATE_FINISHED) {
      stop();
    }
  }
  void key_down(int key) {
    player_.Key(key);
  }

private:
  CPlayer& player_;
};

namespace {
std::filesystem::path parseConfigFileOption(int& argc, char** argv) {
  std::vector<char*> tmpArgs;
  std::filesystem::path cfgFilePath{};
  for (int i = 0; i < argc; i++) {
    if (caseInsensitiveEquals(argv[i], "--configfile")) {
      cfgFilePath = std::filesystem::absolute(argv[++i]);
    } else {
      tmpArgs.push_back(argv[i]);
    }
  }
  argc = static_cast<int>(tmpArgs.size());
  for (int i = 0; i < argc; i++) {
    argv[i] = tmpArgs[i];
  }
  return cfgFilePath;
}

std::set<std::string> argsFilterTags = {};

bool argsFilterTagsFunc(const args::Base& item) {
  if (argsFilterTags.size() <= 0) {
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

    if (std::find_if(argsFilterTags.begin(), argsFilterTags.end(), comparisonFunc) !=
        argsFilterTags.end()) {
      return true;
    }
  }
  return false;
}
} // namespace

int MainBody(int argc, char* argv[]) {
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
    return 1;
  case ParsingSemanticError:
    LOG_ERROR << "Error during command line parsing:\n" << args.Output.str();
    return 1;
  case ShowHelp:
    break;
  }

  if ((args.ParsingResult == ShowHelp) || (args.HelpMenu)) {
    if (args.HelpMenu) {
      argsFilterTags.insert(args.HelpMenu.Get());
      args::GlobalFilterOption = argsFilterTagsFunc;
    }
    std::cout
        << std::endl
        << std::endl
        << args.Parser.Help() << std::endl
        << "All options of a configfile can be set via commandline by using the keypath and value."
        << std::endl;
    return 0;
  }

  if (args.Version) {
    // Print version and quit.
    CGits& inst = CGits::Instance();
    std::cout << inst << std::endl;
    return 0;
  }

  try {
    if (!ConfigurePlayer(playerPath, args)) {
      LOG_ERROR << "Encountered error while configuring player";
      LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
      return 1;
    }
  } catch (const std::exception& e) {
    LOG_ERROR << "Encountered error while configuring player:\n" << e.what();
    LOG_ERROR << "Please run player with the \"--help\" argument to see usage info.";
    return 1;
  }

  // Print version.
  CGits& inst = CGits::Instance();
  LOG_INFO << inst;

  Configurator::Instance().LogChangedFields();

  inst.GetMessageBus().subscribe({PUBLISHER_PLUGIN, TOPIC_LOG}, [](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LogMessage>(m);
    if (msg) {
      PLOG(log::GetSeverity(msg->getLevel())) << msg->getText();
    }
  });

  const auto& cfg = Configurator::Get();
  log::SetMaxSeverity(cfg.common.shared.thresholdLogLevel);
  if (!cfg.common.shared.logToConsole) {
    log::RemoveConsoleAppender();
  }
  if (!cfg.common.player.outputTracePath.empty()) {
    log::AddFileAppender(cfg.common.player.outputTracePath);
  }

  int returnValue = EXIT_SUCCESS;

  try {
    if (cfg.common.player.waitForEnter) {
      LOG_NONE << "Waiting for ENTER press ...";
      std::cin.get();
    }

#if defined GITS_PLATFORM_WINDOWS
    if (cfg.common.player.escalatePriority) {
      if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        LOG_INFO << "Escalated process priority to realtime priority";
      } else {
        LOG_WARNING << "Priority escalation failed";
      }
    }

    int previousDesktopWidth = GetSystemMetrics(SM_CXSCREEN);
    int previousDesktopHeight = GetSystemMetrics(SM_CYSCREEN);
    if (cfg.common.player.forceDesktopResolution.enabled) {
      DEVMODE devmode;
      devmode.dmPelsWidth = cfg.common.player.forceDesktopResolution.width;
      devmode.dmPelsHeight = cfg.common.player.forceDesktopResolution.height;
      ;
      devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmSize = sizeof(DEVMODE);

      ChangeDisplaySettings(&devmode, 0);
    }

#ifdef WITH_VULKAN
    if (cfg.vulkan.player.renderDoc.mode != TVkRenderDocCaptureMode::NONE) {
      if (!cfg.vulkan.player.renderDoc.dllPath.empty()) {
        Vulkan::RenderDocUtil::dllpath = cfg.vulkan.player.renderDoc.dllPath.string();
      } else {
        Vulkan::RenderDocUtil::dllpath = GetRenderDocDllPath();
      }
      Vulkan::RenderDocUtil::GetInstance();
    }
#endif
#endif

    if (cfg.common.shared.useEvents) {
      CGits::Instance().ProcessLuaFunctionsRegistrators();
    }
    // initialize GITS
    LOG_INFO << "Initializing...";
#if WITH_OPENCL
    if (!cfg.opencl.player.noOpenCL) {
      inst.Register(std::shared_ptr<CLibrary>(new OpenCL::CLibrary));
    }
#endif
    inst.Register(std::shared_ptr<CLibrary>(new OpenGL::CLibrary));
#ifdef WITH_VULKAN
    inst.Register(std::shared_ptr<CLibrary>(new Vulkan::CLibrary));
#endif
#ifdef WITH_LEVELZERO
    inst.Register(std::shared_ptr<CLibrary>(new l0::CLibrary));
#endif
#ifdef WITH_OCLOC
    inst.Register(std::shared_ptr<CLibrary>(new ocloc::CLibrary));
#endif
#if defined WITH_DIRECTX
    inst.Register(std::shared_ptr<CLibrary>(new DirectX::DirectXLibrary));
#endif

    // create player
    CPlayer player;
    LOG_INFO << "Loading...";

    // load function calls from a file
    player.Load(cfg.common.player.streamPath);
#if defined WITH_DIRECTX
    if (cfg.directx.features.subcapture.enabled) {
      CGits::Instance().FileRecorder().SetProperty(
          "diag.original_app.name", CGits::Instance().FilePlayer().GetApplicationName());
    }
#endif

    inst.ResourceManagerInit(cfg.common.player.streamDir);
    if (cfg.common.player.diags) {
      std::cout << CGits::Instance().FilePlayer().ReadProperties();
      return 0;
    }

    if (cfg.common.player.stats) {
      // print statistics
      player.StatisticsPrint();
      return 0;
    }

    // register tokens executor
    if (cfg.common.player.faithfulThreading) {
      player.Register(std::make_unique<CSequentialExecutor>());
    } else {
      player.Register(std::make_unique<CAction>());
    }

    // print not supported functions if exist
    player.NotSupportedFunctionsPrint();

#if defined GITS_PLATFORM_WINDOWS && (WITH_DIRECTX || WITH_VULKAN)
    auto pImGuiHUD = std::make_unique<ImGuiHUD>();
    CGits::Instance().SetImGuiHUD(std::move(pImGuiHUD));
#endif
#ifdef GITS_PLATFORM_WINDOWS
    auto pid = _getpid();
    auto processName = gits::GetWindowsProcessName(pid);
#elif defined GITS_PLATFORM_LINUX
    auto pid = getpid();
    auto processName = GetLinuxProcessName(pid);
#endif
    processName = processName.empty() ? "<unknown>" : processName;
#if defined GITS_PLATFORM_WINDOWS && (WITH_DIRECTX || WITH_VULKAN)
    CGits::Instance().GetImGuiHUD()->SetApplicationInfo(processName, pid);
#endif

    // check if all functions can be run on that system
    LOG_INFO << "Playing...";

    if (Configurator::Get().common.shared.useEvents) {
      try {
        CGits::Instance().PlaybackEvents().programStart();
      } catch (std::runtime_error& e) {
        LOG_ERROR << e.what();
      }
    }

    // process events - enter message loop
    GitsMessagePump pump(player);

    int64_t tillInitTime = CGits::Instance().Timers().program.Get();
    CGits::Instance().Timers().init.Restart();
    pump.process_messages();
    player.GLResourceCleanup();
    player.GLContextsCleanup();

    int64_t playbackTime = CGits::Instance().Timers().playback.Get();
    int64_t initTime = CGits::Instance().Timers().init.Get();
    int64_t restorationTime = CGits::Instance().Timers().restoration.Get();
    int64_t loadingTime = CGits::Instance().Timers().loading.Get();
    int64_t programTime = CGits::Instance().Timers().program.Get();

    LOG_INFO << "";
    LOG_INFO << "Startup time: " << tillInitTime / 1e6 << "ms";
    LOG_INFO << "Initialized in: " << initTime / 1e6 << "ms";
    LOG_INFO << "State restored in: " << restorationTime / 1e6 << "ms";
    LOG_INFO << "Stalled loading: " << loadingTime / 1e6 << "ms";
    LOG_INFO << "Played back in: " << playbackTime / 1e6 << "ms";
    LOG_INFO << "Total runtime: " << programTime / 1e6 << "ms";

    if (gits::CGits::Instance().apis.HasCompute()) {
      gits::CGits::Instance().apis.IfaceCompute().PrintMaxLocalMemoryUsage();
    }

    // Writes performance results to .csv file
    if (cfg.common.player.benchmark) {
      std::filesystem::path outBench = cfg.common.player.outputDir.empty()
                                           ? cfg.common.player.applicationPath
                                           : cfg.common.player.outputDir;
      std::filesystem::create_directories(outBench);
      outBench /= "benchmark.csv";
      std::ofstream timeDataFile(outBench, std::ios::binary | std::ios::out);
      CGits::Instance().TimeSheet().OutputTimeData(timeDataFile, true);
    }

    // Close OpenGL programs zip file
    CGits::Instance().CloseUnZipFileGLPrograms();

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

  if (Configurator::Get().common.shared.useEvents) {
    try {
      CGits::Instance().PlaybackEvents().programExit();
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      returnValue = EXIT_FAILURE;
    }
  }

#if defined GITS_PLATFORM_WINDOWS
  if (Configurator::Get().directx.features.subcapture.enabled &&
      CRecorder::Instance().IsMarkedForDeletion()) {
    CRecorder::Instance().Close();
  }
#endif
  CGits::Instance().Dispose();

  if (Configurator::Get().common.player.waitForEnter) {
    LOG_NONE << "Waiting for ENTER press ...";
    std::cin.get();
  }

  return returnValue;
}

#ifdef GITS_PLATFORM_WINDOWS
void ShowCallstack(PEXCEPTION_POINTERS exceptionPtr) {
  class StackWalkerToConsole : public StackWalker {
  public:
    StackWalkerToConsole() : StackWalker(OptionsAll, ".") {}
    virtual void OnOutput(LPCSTR szText) {
      LOG_ERROR << szText;
    }
  } sw;
  sw.ShowCallstack(GetCurrentThread(), exceptionPtr->ContextRecord);
}
LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exceptionPtr) {
  ShowExceptionInfo(exceptionPtr);
  ShowCallstack(exceptionPtr);
  return Configurator::Get().common.player.disableExceptionHandling ? EXCEPTION_CONTINUE_SEARCH
                                                                    : EXCEPTION_EXECUTE_HANDLER;
}
#endif
} // namespace gits

int main2(int argc, char* argv[]) {
  using namespace gits;
#ifdef GITS_PLATFORM_WINDOWS
#if _DEBUG
  MessageBox(0, "Waiting for debugger...", "Waiting for debugger...", 0);
#endif
  // Prevent OS from scaling our windows.
  SetProcessDPIAware();
#ifdef _NDEBUG
  // This is a workaround for older fullscreen streams that most probably cause
  // heap corruption without noticeable effect until program termination - after
  // leaving main, during process cleanup access violation results I'll leave
  // this in debug builds - as this should not happen for new streams
  SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT |
               SEM_FAILCRITICALERRORS);
#endif

  PEXCEPTION_POINTERS exceptionPtr = 0;
  __try {
    return MainBody(argc, argv);
  } __except (ExceptionFilter(GetExceptionInformation())) {
    return 1;
  }
#else
  return MainBody(argc, argv);
#endif
}

// Normal program entry point.
int main(int argc, char* argv[]) {
  try {
    return main2(argc, argv);
  } catch (...) {
    topmost_exception_handler("main");
  }
}
