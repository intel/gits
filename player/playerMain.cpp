// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

#include "player.h"
#include "exception.h"
#include "config.h"
#include "log.h"
#include "performance.h"
#include "windowing.h"
#include "timer.h"
#include "runner.h"
#include "sequentialExecutor.h"
#include "pragmas.h"
#include "playerOptions.h"
#include "hashing.h"
#include "message_pump.h"

#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <filesystem>

DISABLE_WARNINGS
#include <boost/property_tree/info_parser.hpp>
ENABLE_WARNINGS

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

int MainBody(int argc, char* argv[]) {
  try {
    if (configure_player(argc, argv)) {
      return 0;
    }
  } catch (const std::runtime_error& e) {
    Log(ERR) << "Error during command line parsing:\n" << e.what();
    Log(ERR) << "Please run player with the \"--help\" argument to see usage info.";
    return 1;
  }
  const Config& cfg = Config::Get();

  // Print version.
  CGits& inst = CGits::Instance();
  Log(INFO, NO_PREFIX) << inst << "\n";

  if (!cfg.player.outputTracePath.empty()) {
    CLog::LogFilePlayer(cfg.player.outputTracePath);
  }

  int returnValue = EXIT_SUCCESS;

  try {
    if (cfg.player.signStream) {
      sign_directory(cfg.common.streamDir);
      Log(INFO) << " (Note that all files in the stream folder and its"
                   " subfolders will be treated as stream files and signed unless they are"
                   " on GITS' internal blacklist. If necessary, you can edit the signature"
                   " file by hand to remove stray non-stream files from it.)";
      return 0;
    }

    if (cfg.player.dontVerifyStream) {
      Log(WARN) << "Skipping stream signature verification because --dontVerifyStream was used.";
    } else {
      verify_directory(cfg.common.streamDir);
    }
    if (cfg.player.verifyStream) {
      Log(WARN) << "Skipping stream playback because --verifyStream was used.";
      return 0;
    }

    if (cfg.player.waitForEnter) {
      Log(OFF, NO_PREFIX) << "Waiting for ENTER press ...";
      std::cin.get();
    }

#if defined GITS_PLATFORM_WINDOWS
    if (cfg.player.escalatePriority) {
      if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        Log(INFO) << "Escalated process priority to realtime priority";
      } else {
        Log(WARN) << "Priority escalation failed";
      }
    }

    int previousDesktopWidth = GetSystemMetrics(SM_CXSCREEN);
    int previousDesktopHeight = GetSystemMetrics(SM_CYSCREEN);
    if (cfg.player.forceDesktopResolution) {
      DEVMODE devmode;
      devmode.dmPelsWidth = cfg.player.forcedDesktopResolution[0];
      devmode.dmPelsHeight = cfg.player.forcedDesktopResolution[1];
      ;
      devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmSize = sizeof(DEVMODE);

      ChangeDisplaySettings(&devmode, 0);
    }

    if (cfg.player.renderDoc.frameRecEnabled || cfg.player.renderDoc.queuesubmitRecEnabled) {
      if (!cfg.player.renderDoc.dllPath.empty()) {
        Vulkan::RenderDocUtil::dllpath = cfg.player.renderDoc.dllPath.string();
      } else {
        Vulkan::RenderDocUtil::dllpath = GetRenderDocDllPath();
      }
      Vulkan::RenderDocUtil::GetInstance();
    }
#endif

    if (cfg.common.useEvents) {
      CGits::Instance().ProcessLuaFunctionsRegistrators();
    }
    // initialize GITS
    Log(INFO, NO_PREFIX) << "Initializing...";
#if WITH_OPENCL
    if (!cfg.player.noOpenCL) {
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

    // create player
    CPlayer player;

    if (cfg.player.precacheResources) {
      Log(INFO, NO_PREFIX) << "\nPrecaching ...";

      Timer precache;
      precache_resources(cfg.common.streamDir);
      Log(INFO, NO_PREFIX) << "Precaching completed in " << precache.Get() / 1e6 << "ms";
    }

    Log(INFO, NO_PREFIX) << "\nLoading...";

    // load function calls from a file
    player.Load(cfg.player.streamPath);

    inst.ResourceManagerInit(cfg.common.streamDir);

    if (cfg.player.diags) {
      write_info(std::cout, CGits::Instance().File().GetPropertyTree());
      return 0;
    }

    if (cfg.player.stats) {
      // print statistics
      player.StatisticsPrint(cfg.player.statsVerb);
      return 0;
    }

    // register tokens executor
    if (cfg.player.faithfulThreading) {
      player.Register(std::make_unique<CSequentialExecutor>());
    } else {
      player.Register(std::make_unique<CAction>());
    }

    // print not supported functions if exist
    player.NotSupportedFunctionsPrint();

    // check if all functions can be run on that system
    Log(INFO, NO_PREFIX) << "Playing...";

    if (Config::Get().common.useEvents) {
      try {
        CGits::Instance().PlaybackEvents().programStart();
      } catch (std::runtime_error& e) {
        Log(ERR) << e.what();
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

    Log(INFO, NO_PREFIX) << "";
    Log(INFO, NO_PREFIX) << "Startup time: " << tillInitTime / 1e6 << "ms";
    Log(INFO, NO_PREFIX) << "Initialized in: " << initTime / 1e6 << "ms";
    Log(INFO, NO_PREFIX) << "State restored in: " << restorationTime / 1e6 << "ms";
    Log(INFO, NO_PREFIX) << "Stalled loading: " << loadingTime / 1e6 << "ms";
    Log(INFO, NO_PREFIX) << "Played back in: " << (playbackTime - loadingTime) / 1e6 << "ms";
    Log(INFO, NO_PREFIX) << "Total runtime: " << programTime / 1e6 << "ms";

    if (gits::CGits::Instance().apis.HasCompute()) {
      gits::CGits::Instance().apis.IfaceCompute().PrintMaxLocalMemoryUsage();
    }

    // Writes performance results to .csv file
    if (cfg.player.benchmark) {
      std::filesystem::path outBench =
          cfg.player.outputDir.empty() ? cfg.player.applicationPath : cfg.player.outputDir;
      std::filesystem::create_directories(outBench);
      outBench /= "benchmark.csv";
      std::ofstream timeDataFile(outBench, std::ios::binary | std::ios::out);
      CGits::Instance().TimeSheet().OutputTimeData(timeDataFile, true);
    }

    // Close OpenGL programs zip file
    CGits::Instance().CloseUnZipFileGLPrograms();

#ifdef GITS_PLATFORM_WINDOWS
    if (cfg.player.forceDesktopResolution) {
      DEVMODE devmode;
      devmode.dmPelsWidth = previousDesktopWidth;
      devmode.dmPelsHeight = previousDesktopHeight;
      devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmSize = sizeof(DEVMODE);

      ChangeDisplaySettings(&devmode, 0);
    }
#endif

    Log(INFO, NO_PREFIX) << "Finishing...";
  } catch (Exception& ex) {
    Log(ERR) << ex.what();
    returnValue = EXIT_FAILURE;
  } catch (std::exception& ex) {
    Log(ERR) << ex.what();
    returnValue = EXIT_FAILURE;
  } catch (...) {
    Log(ERR) << "Unrecognized exception was raised during GITS execution!!!";
    returnValue = EXIT_FAILURE;
  }

  if (Config::Get().common.useEvents) {
    try {
      CGits::Instance().PlaybackEvents().programExit();
    } catch (std::runtime_error& e) {
      Log(ERR) << e.what();
      returnValue = EXIT_FAILURE;
    }
  }

  CGits::Instance().Dispose();

  if (Config::Get().player.waitForEnter) {
    Log(OFF, NO_PREFIX) << "Waiting for ENTER press ...";
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
      Log(ERR) << szText;
    }
  } sw;
  sw.ShowCallstack(GetCurrentThread(), exceptionPtr->ContextRecord);
}
LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exceptionPtr) {
  ShowExceptionInfo(exceptionPtr);
  ShowCallstack(exceptionPtr);
  return Config::Get().player.disableExceptionHandling ? EXCEPTION_CONTINUE_SEARCH
                                                       : EXCEPTION_EXECUTE_HANDLER;
}
#endif
} // namespace gits

int main2(int argc, char* argv[]) {
  using namespace gits;
#ifdef GITS_PLATFORM_WINDOWS
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
