// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gitsApi.h"
#include "helper.h"
#include "config.h"
#include "gits.h"
#include "resource_manager.h"
#include "platform.h"
#include "getopt_.h"
#include "openglDrivers.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#ifdef GITS_API_OGL
#include "windowing.h"
#include "windowContextState.h"
#endif
#ifdef GITS_API_VK
#include "helperVk.h"
#endif

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

#include <filesystem>

namespace gits {}
using namespace gits;
using gits::OpenGL::drv;

#include <stdlib.h>
#include <iostream>

#define WIDTH  1600
#define HEIGHT 1024

#ifndef _EMPTY_BUILD
void RunFrames();
#endif

#ifdef _TIMING_MASK_AND_FRAME_LOOP_
extern int blockTimingMask;
extern int runCycles;
#endif

#ifdef _PERF_MODE_
char* gitsRawBuffer;

void LoadGitsRawFile(const std::string& fileName) {
  std::ifstream file(fileName.c_str(), std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << fileName << "\n";
    exit(-1);
  }

  file.seekg(0, std::ios::end);
  size_t fileLength = file.tellg();
  file.seekg(0, std::ios::beg);

  gitsRawBuffer = new char[fileLength];
  file.read(gitsRawBuffer, fileLength);

  if (!file) {
    std::cerr << "Failed reading: " << fileName << "\n";
  }
}
#endif

int main(int argc, char* argv[]) {
  try {
    // command line options parser
    CGetOpt options(argc, argv);

    TypedOption<bool> optionHelp(options, OPTION_GROUP_GENERAL, 'h', "help",
                                 "Displays this help message.");

    TypedOption<BitRange> optionCaptureFrames(
        options, OPTION_GROUP_GENERAL, 0, "captureFrames",
        "List of frames that will be captured "
        "during playback. Specified as single string without spaces of "
        "following format:\n"
        "  * pattern = frame_list | frame_list/Repeat-Count\n"
        "  * frame_list = element | frame_list, ',', element\n"
        "  * element = Frame | Begin-End | Begin-End:Step");

    TypedOption<BitRange> optionDumpDraws(
        options, OPTION_GROUP_GENERAL, 0, "dumpDraws",
        "List of draws that will be captured "
        "during playback. Specified as single string without spaces of "
        "following format:\n"
        "  * pattern = frame_list | frame_list/Repeat-Count\n"
        "  * frame_list = element | frame_list, ',', element\n"
        "  * element = Frame | Begin-End | Begin-End:Step");

    TypedOption<bool> optionDumpDrawsPre(
        options, OPTION_GROUP_GENERAL, 0, "dumpDrawsPre",
        "Causes 'dumpDraws' to capture "
        "drawbuffer content not only after specified drawcall, but also before it.");
    TypedOption<std::filesystem::path> optionOutputDir(
        options, OPTION_GROUP_GENERAL, 0, "outputDir",
        "Specifies directory where all the artifacts "
        "will be stored.");

    TypedOption<std::filesystem::path> optionStreamDir(
        options, OPTION_GROUP_GENERAL, 0, "streamDir",
        "Specifies stream resources location - should be "
        "specified if it is not a current working directory");

    TypedOption<bool> optionWaitForEnter(
        options, OPTION_GROUP_GENERAL, 0, "waitForEnter",
        "Before and after playing back the stream, wait for ENTER keypress.");

    TypedOption<bool> optionCaptureWholeWindow(options, OPTION_GROUP_GENERAL, 0,
                                               "captureWholeWindow",
                                               "Causes '--screenshots' to capture whole "
                                               "current window instead of current viewport.",
                                               GITS_PLATFORM_BIT_WINDOWS);

    TypedOption<bool> optionCaptureFinishFrame(options, OPTION_GROUP_GENERAL, 0,
                                               "captureFinishFrame",
                                               "Capture framebuffer content on glFinish.");

    TypedOption<bool> optionCaptureReadPixels(options, OPTION_GROUP_GENERAL, 0, "captureReadPixels",
                                              "Capture framebuffer content on glReadPixels.");

    TypedOption<bool> optionCaptureFlushFrame(options, OPTION_GROUP_GENERAL, 0, "captureFlushFrame",
                                              "Capture framebuffer content on glFlush.");

    TypedOption<unsigned> optionUseVKPhysicalDeviceIndex(
        options, OPTION_GROUP_PLAYBACK, 0, "useVKPhysicalDeviceIndex",
        "Forces selected physical device index to be used for logical device creation.",
        GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

    TypedOption<unsigned> optionBenchmarkStartFrame(
        options, OPTION_GROUP_GENERAL, 0, "benchmarkStartFrame",
        "Benchmarking will start on this frame. All "
        "previous frames will be ignored. This option exists because few first "
        "frames are typically much slower than the rest and they significantly "
        "decrease the average FPS number. Default value: 3");

    TypedOption<bool> optionDisableLogging(
        options, OPTION_GROUP_PERFORMANCE, 0, "disableLogging",
        "Disable console output, which should slighly increase performance.");

    TypedOption<bool> optionEscalatePriority(options, OPTION_GROUP_PERFORMANCE, 0, "realtime",
                                             "If specified, CCodeProject will request "
                                             "realtime priority from the system.");

    // If we  add some performance options in the future, we should include them in this option.
    TypedOption<bool> optionPerformance(
        options, OPTION_GROUP_PERFORMANCE, 0, "performance",
        "Enable all the performance options (disableLogging, realtime). "
        "Recommended for performance tests.");

    options.Parse();

    Config cfg = Config::Get();
    if (optionCaptureFrames.Present()) {
      cfg.common.player.captureFrames = optionCaptureFrames.Value();
    }

    if (optionDumpDraws.Present()) {
      cfg.opengl.player.captureDraws = optionDumpDraws.Value();
    }

    if (optionDumpDrawsPre.Present()) {
      cfg.opengl.player.captureDrawsPre = optionDumpDrawsPre.Value();
    }

    //if(optionDumpDraws.Present())
    //  cfg.player.captureDraws = BitRange(optionDumpDraws.Value());

    if (optionHelp.Present()) {
      options.Usage("all");
      return 0;
    }
    if (optionOutputDir.Present()) {
      cfg.ccode.outputPath = std::filesystem::absolute(optionOutputDir.Value());
    } else {
      cfg.ccode.outputPath = std::filesystem::current_path();
    }

    // CCode for Vulkan uses functions from vulkanTools.cpp (common code) to
    // dump screenshots. Therefore it depends on player.outputDir instead of
    // ccode.outputPath:
    cfg.common.player.outputDir = cfg.ccode.outputPath;

    if (optionStreamDir.Present()) {
      cfg.common.player.streamDir = std::filesystem::absolute(optionStreamDir.Value()) / "";
    }

    if (optionWaitForEnter.Present()) {
      cfg.common.player.waitForEnter = optionWaitForEnter.Value();
    }

    cfg.opengl.player.captureWholeWindow = optionCaptureWholeWindow.Value();

    if (optionCaptureFinishFrame.Present()) {
      cfg.opengl.player.captureFinishFrame = BitRange(true);
    }

    if (optionCaptureReadPixels.Present()) {
      cfg.opengl.player.captureReadPixels = BitRange(true);
    }

    if (optionCaptureFlushFrame.Present()) {
      cfg.opengl.player.captureFlushFrame = BitRange(true);
    }

    if (optionUseVKPhysicalDeviceIndex.Present()) {
      cfg.vulkan.player.vulkanForcedPhysicalDeviceIndex = optionUseVKPhysicalDeviceIndex.Value();
    }

    if (optionBenchmarkStartFrame.Present()) {
      cfg.ccode.benchmarkStartFrame = optionBenchmarkStartFrame.Value();
    } else {
      cfg.ccode.benchmarkStartFrame = 3; //default value
    }

    if (optionDisableLogging.Present() || optionPerformance.Present()) {
      cfg.common.shared.thresholdLogLevel = LogLevel::OFF;
      // Log can't use config directly, see log.cpp for info.
      CLog::SetLogLevel(cfg.common.shared.thresholdLogLevel);
    }

    if (optionEscalatePriority.Present() || optionPerformance.Present()) {
      cfg.common.player.escalatePriority = true;
    }

    Config::Set(cfg);
  } catch (const std::runtime_error& e) {
    std::cout << "Error during command line parsing:\n" << e.what() << "\n";
    return 1;
  } catch (...) {
    topmost_exception_handler("main");
  }

  try {
    if (Config::Get().common.player.waitForEnter) {
      Log(INFO) << "Waiting for ENTER press ...";
      std::cin.get();
    }

#ifdef _PERF_MODE_
    LoadGitsRawFile("gitsData.raw");
#endif

#ifdef _TIMING_MASK_AND_FRAME_LOOP_
    blockTimingMask = (argc >= 2) ? atoi(argv[1]) : blockTimingMask;
    runCycles = (argc == 3) ? atoi(argv[2]) : runCycles;
#endif

#if defined WITH_LEVELZERO and defined GITS_API_L0
    InitL0();
#endif

#if defined GITS_API_OCL && !defined CCODE_FOR_EGL
    CLInit();
#endif

#if defined GITS_API_VK
    InitVk();
#endif

#if defined GITS_PLATFORM_WINDOWS
    if (Config::Get().common.player.escalatePriority) {
      if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        Log(INFO) << "Escalated process priority to realtime priority";
      } else {
        Log(WARN) << "Priority escalation failed";
      }
    }
    SetProcessDPIAware();
#endif
  } catch (...) {
    topmost_exception_handler("main - Initialization");
  }

  try {
#ifndef _EMPTY_BUILD
    RunFrames();
#endif
  } catch (const std::runtime_error& e) {
    std::cout << "Runtime Error: " << e.what() << "\n";
  }
  try {
    uint32_t currentFrame = CGits::Instance().CurrentFrame();
    Log(OFF, NO_PREFIX) << "Rendered frames : " << currentFrame;
    if (currentFrame > Config::Get().ccode.benchmarkStartFrame) {
      int64_t timeElapsed = CGits::Instance().GetLastFrameTime();
      float averageFPS = CGits::Instance().GetFPS();
      Log(OFF, NO_PREFIX) << "Rendering Time: " << timeElapsed / 1e9
                          << " s; Average FPS: " << averageFPS;
    }

#if defined GITS_API_VK
    ReleaseVk();
#endif

    Log(OFF, NO_PREFIX) << "Finished";

    if (Config::Get().common.player.waitForEnter) {
      Log(WARN) << "Waiting for ENTER press ...";
      std::cin.get();
    }

#ifdef _PERF_MODE_
    if (gitsRawBuffer) {
      delete[] gitsRawBuffer;
    }
#endif
  } catch (...) {
    topmost_exception_handler("main - Finishing");
  }

  return 0;
}
