// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "config.h"
#include "keyEvents.h"

#ifndef BUILD_FOR_CCODE
#include "diagnostic.h"
#include "lua_bindings.h"
#endif

#include <regex>
#include <chrono>

DISABLE_WARNINGS
#include <boost/property_tree/info_parser.hpp>
#include <boost/filesystem.hpp>
ENABLE_WARNINGS

#ifdef GITS_PLATFORM_WINDOWS
#include <process.h>
#define getpid _getpid
#endif

using namespace std;

namespace {
std::string read_file(const boost::filesystem::path& file_name) {
  boost::filesystem::ifstream file(file_name, std::ios::binary | std::ios::in);
  if (file.is_open()) {
    std::stringstream str;
    str << file.rdbuf();
    return str.str();
  }
  return "";
}
} // namespace

gits::Config* gits::Config::config = new Config;

gits::Config::Config() {
  common.mode = MODE_UNKNOWN;
  recorder.openGL.capture.all.exitFrame = 100000000;

  common.tokenBurst = 1500000;
  common.tokenBurstNum = 5;

  player.windowCoords = std::make_pair(0, 0);
  player.windowSize = std::make_pair(0, 0);
  player.keepDraws = BitRange(true);
  player.keepApis = BitRange(true);
  player.keepFrames = BitRange(true);
  player.scaleFactor = 1.0f;
  player.tokenLoadLimit = UINT_MAX;
  player.maxAllowedVkSwapchainRewinds = 100;
  player.vulkanForcedPhysicalDeviceIndex = 0;
  player.vulkanForcedPhysicalDeviceName = "";
  player.vulkanForcedPhysicalDeviceType.setFromString("any");
  player.printStateRestoreLogsVk = false;
  player.printMemUsageVk = false;
  player.forceMultithreadedPipelineCompilation = false;

  recorder.extras.optimizations.partialHash.cutoff = 8192;
  recorder.extras.optimizations.partialHash.chunks = 10;
  recorder.extras.optimizations.partialHash.ratio = 20;
  recorder.extras.utilities.extendedDiagnosticInfo = true;
  recorder.basic.exitSignal = 15;

  recorder.openGL.utilities.updateMappedTexturesEveryNSwaps = 1;
  recorder.openGL.utilities.stripIndicesValues = 0xFFFFFFFF;
  recorder.openGL.utilities.restoreDefaulFB = true;
  recorder.openGL.utilities.restoreFBFrontAndBackWA = false;
  recorder.openGL.utilities.doNotRemoveWin = false;
  recorder.openGL.utilities.detectRecursion = false;
  recorder.openGL.utilities.suppressProgramBinary = true;
  recorder.openGL.utilities.trackTextureBindingWA = false;
  recorder.openGL.utilities.forceBuffersStateCaptureAlwaysWA = false;
  recorder.openGL.utilities.restoreIndexedTexturesWA = false;
  recorder.openGL.utilities.forceFBOSupportWA = false;
  recorder.openGL.utilities.retryFunctionLoads = true;

  recorder.vulkan.utilities.forceUniversalRecording = false;
  recorder.vulkan.utilities.useExternalMemoryExtension = false;
  recorder.vulkan.utilities.reusableStateRestoreResourcesCount = 3;
  recorder.vulkan.utilities.reusableStateRestoreBufferSize = 80;
  recorder.vulkan.utilities.maxArraySizeForCCode = 400;

  recorder.extras.utilities.closeAppOnStopRecording = true;

#if defined GITS_PLATFORM_WINDOWS
  common.libGL = "OpenGL32.dll";
  common.libEGL = "libEGL.dll";
  common.libGLESv1 = "libGLESv1_CM.dll";
  common.libGLESv2 = "libGLESv2.dll";
  common.libClPath = "OpenCL.dll";
  common.libVK = "vulkan-1.dll";
  common.libOcloc = "ocloc64.dll";
  common.libL0Driver = "ze_intel_gpu64.dll";
  common.libL0 = "ze_loader.dll";
#elif defined GITS_PLATFORM_X11
  common.libGL = "libGL.so.1";
  common.libEGL = "libEGL.so.1";
  common.libGLESv1 = "libGLESv1_CM.so.1";
  common.libGLESv2 = "libGLESv2.so.2";
  common.libClPath = "libOpenCL.so.1";
  common.libVK = "libvulkan.so.1";
  common.libOcloc = "libocloc.so";
  common.libL0Driver = "libze_intel_gpu.so.1";
  common.libL0 = "libze_loader.so.1";
#else
#error Invalid platform.
#endif
}

void gits::Config::Set(const Config& cfg) {
  Config& config_ = const_cast<Config&>(Get());
  config_ = cfg;
}

#ifndef BUILD_FOR_CCODE

bool gits::Config::Set(const boost::filesystem::path& cfgDir) {
  static bool configured = false;
  if (configured) {
    return true;
  }

  boost::property_tree::ptree pt;
  auto cfgPath = cfgDir / "gits_config.txt";
  GetConfigPtree(cfgPath, pt);
  auto cfg = Config::Get();

  //****************************** BASIC ****************************************
  cfg.common.mode = MODE_RECORDER;
  ReadRecorderOption(pt, "Basic.RecordingEnabled", cfg.recorder.basic.enabled,
                     GITS_PLATFORM_BIT_ALL);

  // Log level handling.
  cfg.common.thresholdLogLevel = LogLevel::INFO; // default
  bool traceWasPresent = false;
  ReadRecorderOption(pt, "Basic.Trace", traceWasPresent, GITS_PLATFORM_BIT_ALL, true);
  if (traceWasPresent) {
    cfg.common.thresholdLogLevel = LogLevel::TRACE;
    Log(WARN) << "The Basic.Trace configuration option is deprecated. "
              << "Please use Basic.LogLevel set to \"TRACE\" instead.";
  }
  std::string logLevelString;
  ReadRecorderOption(pt, "Basic.LogLevel", logLevelString, GITS_PLATFORM_BIT_ALL);
  if (!logLevelString.empty()) {
    CEnumParser<LogLevel> logLevelParser;
    boost::optional<LogLevel> lvl = logLevelParser.ParseEnum(logLevelString);
    if (lvl) {
      cfg.common.thresholdLogLevel = lvl.get();
    } else {
      Log(ERR) << "Incorrect log level: \"" << logLevelString << "\".";
      throw std::invalid_argument(EXCEPTION_MESSAGE);
    }
  }
  CLog::SetLogLevel(
      cfg.common.thresholdLogLevel); // Log can't use config directly, see log.cpp for info.
  cfg.recorder.basic.dumpGITS = true;
  cfg.recorder.basic.dumpCCode = false;
  ReadRecorderOption(pt, "Basic.BinaryDump", cfg.recorder.basic.dumpGITS, GITS_PLATFORM_BIT_ALL,
                     true);
  ReadRecorderOption(pt, "Basic.CCodeDump", cfg.recorder.basic.dumpCCode, GITS_PLATFORM_BIT_ALL,
                     true);

  if (!cfg.recorder.basic.enabled &&
      (cfg.recorder.basic.dumpGITS || cfg.recorder.basic.dumpCCode)) {
    Log(ERR) << "Dump options cannot be enabled without setting recorderEnabled to True";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }

  if (cfg.recorder.basic.enabled && !cfg.recorder.basic.dumpGITS && !cfg.recorder.basic.dumpCCode) {
    cfg.recorder.basic.enabled = false;
    Log(INFO) << "Dump option not selected. Setting recordingEnabled to False";
  }

  ReadRecorderOption(pt, "Basic.ExitSignal", cfg.recorder.basic.exitSignal, GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Basic.ExitAfterAPICall", cfg.recorder.basic.exitAfterAPICall,
                     GITS_PLATFORM_BIT_ALL);
  std::vector<unsigned> exitKeysCodes;
  std::vector<std::string> exitkeys;
  ReadRecorderOption(pt, "Basic.ExitKeys", exitkeys, GITS_PLATFORM_BIT_ALL);
  if (!exitkeys.empty()) {
    std::transform(exitkeys.begin(), exitkeys.end(), std::back_inserter(exitKeysCodes), GetKeyVal);
    if (std::find(exitKeysCodes.begin(), exitKeysCodes.end(), 0U) != exitKeysCodes.end()) {
      Log(ERR) << "Invalid startup key combination given.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  cfg.recorder.basic.exitKeys.swap(exitKeysCodes);

  ReadRecorderOption(pt, "Basic.Paths.LibGL", cfg.common.libGL, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.LibEGL", cfg.common.libEGL, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.LibGLES1", cfg.common.libGLESv1, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.LibGLES2", cfg.common.libGLESv2, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.LibCL", cfg.common.libClPath, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.LibVK", cfg.common.libVK,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
#ifdef WITH_LEVELZERO
  ReadRecorderOption(pt, "Basic.Paths.LibL0", cfg.common.libL0, GITS_PLATFORM_BIT_ALL);
#endif
#ifdef WITH_OCLOC
  ReadRecorderOption(pt, "Basic.Paths.LibOcloc", cfg.common.libOcloc, GITS_PLATFORM_BIT_ALL);
#endif
  ReadRecorderOption(pt, "Basic.Paths.InstallationPath", cfg.common.installPath,
                     GITS_PLATFORM_BIT_ALL);

  std::string dir;
  ReadRecorderOption(pt, "Basic.Paths.UniqueDumpDirectory",
                     cfg.recorder.basic.paths.uniqueDumpDirectory, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Basic.Paths.DumpDirectoryPath", dir, GITS_PLATFORM_BIT_ALL);
  {
    if (!dir.empty()) {
      static bool postfixSet = false;
      if (cfg.recorder.basic.paths.uniqueDumpDirectory && !postfixSet) {
        // postfix with a timestamp
        using namespace std::chrono;
        auto time = system_clock::now();

        time_t t = system_clock::to_time_t(time);
        char text[256];
        strftime(text, sizeof(text), "%y_%m_%d_%H_%M_%S", localtime(&t));

        milliseconds ms = duration_cast<milliseconds>(time.time_since_epoch()) -
                          duration_cast<seconds>(time.time_since_epoch());

        dir += "_" + std::string(text) + "." + std::to_string(ms.count());
        postfixSet = true;
      }
    }

    //handle special names in dump dir path:
    //  move to somewhere in common if this grows
    //  %p% - process pid
    //  %n% - process name
    const char pidPlaceholder[] = "%p%";
    std::string::size_type pidPos = dir.find(pidPlaceholder);
    const char namePlaceholder[] = "%n%";
    std::string::size_type namePos = dir.find(namePlaceholder);

    if (pidPos != std::string::npos) {
      std::stringstream str;
      str << getpid();

      std::string left = dir.substr(0, pidPos);
      std::string right = dir.substr(pidPos + sizeof(pidPlaceholder) - 1);
      dir = left + str.str() + right;
    }

    if (namePos != std::string::npos) {
      std::string processname = "";
#ifdef GITS_PLATFORM_WINDOWS
      int pid = _getpid();
      processname += GetWindowsProcessName(pid);
#elif defined GITS_PLATFORM_LINUX
      pid_t pid = getpid();
      processname += GetLinuxProcessName(pid);
#endif

      std::string left = dir.substr(0, namePos);
      std::string right = dir.substr(namePos + sizeof(namePlaceholder) - 1);
      dir = left + processname + right;
    }
    cfg.common.streamDir = dir;
  }

  //****************************** OPENGL ****************************************
  ReadRecorderOption(pt, "OpenGL.Capture.Mode", cfg.recorder.openGL.capture.mode,
                     GITS_PLATFORM_BIT_ALL);
  {
    //Frames
    int startFrame = 0;
    int stopFrame = 0;
    std::vector<unsigned> startKeyCodes;
    ReadRecorderOption(pt, "OpenGL.Capture.Frames.StartFrame", startFrame, GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.Frames.StopFrame", stopFrame, GITS_PLATFORM_BIT_ALL);
    std::vector<std::string> startkeys;
    ReadRecorderOption(pt, "OpenGL.Capture.Frames.StartKeys", startkeys, GITS_PLATFORM_BIT_ALL);
    if (!startkeys.empty()) {
      std::transform(startkeys.begin(), startkeys.end(), std::back_inserter(startKeyCodes),
                     GetKeyVal);
      if (std::find(startKeyCodes.begin(), startKeyCodes.end(), 0U) != startKeyCodes.end()) {
        Log(ERR) << "Invalid GL startup key combination given.";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }
    if (stopFrame == 0) {
      //0 has a special meaning of the longest possible capture
      stopFrame = static_cast<unsigned>(-1);
    }
    if (startFrame > stopFrame) {
      Log(ERR) << "StartFrame can't be greater than StopFrame";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startFrame == 0) {
      Log(ERR) << "StartFrame can't be 0";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    //Draw
    int number;
    ReadRecorderOption(pt, "OpenGL.Capture.OglSingleDraw.Number", number, GITS_PLATFORM_BIT_ALL);
    if (number < 0) {
      Log(ERR) << "Number option can't be less then zero";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    int startDraw = 0;
    int stopDraw = 0;
    int frame = 0;
    uint exitFrameOpenGL = 0;
    int exitDeleteContext = 0;
    vi_bool glFinishSep;
    vi_bool glFlushSep;
    ReadRecorderOption(pt, "OpenGL.Capture.OglDrawsRange.StartDraw", startDraw,
                       GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.OglDrawsRange.StopDraw", stopDraw,
                       GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.OglDrawsRange.Frame", frame, GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.All.ExitFrame", exitFrameOpenGL, GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.All.ExitDeleteContext", exitDeleteContext,
                       GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.Frames.FrameSeparators.glFinish", glFinishSep,
                       GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenGL.Capture.Frames.FrameSeparators.glFlush", glFlushSep,
                       GITS_PLATFORM_BIT_ALL);
    if (stopDraw == 0) {
      //0 has a special meaning of the longest possible capture
      stopDraw = static_cast<unsigned>(-1);
    }
    if (startDraw > stopDraw) {
      Log(ERR) << "StartDraw can't be greater than StopDraw";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startDraw == 0) {
      Log(ERR) << "StartDraw can't be 0";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    //Init default
    cfg.recorder.openGL.capture.frames.startFrame = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.frames.stopFrame = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.all.exitFrame = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.oglSingleDraw.number = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.oglDrawsRange.startDraw = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.oglDrawsRange.stopDraw = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.oglDrawsRange.frame = static_cast<unsigned>(-1);
    cfg.recorder.openGL.capture.frames.frameSeparators.glFinishSep = false;
    cfg.recorder.openGL.capture.frames.frameSeparators.glFlushSep = false;
    cfg.recorder.openGL.capture.all.exitDeleteContext = 0;

    //Choose frame range basing on mode and params
    if (cfg.recorder.openGL.capture.mode.find("Frames") != std::string::npos) {
      cfg.recorder.openGL.capture.frames.startFrame = startFrame;
      cfg.recorder.openGL.capture.frames.stopFrame = stopFrame;
      cfg.recorder.openGL.capture.frames.startKeys.swap(startKeyCodes);
      cfg.recorder.openGL.capture.frames.frameSeparators.glFinishSep = glFinishSep;
      cfg.recorder.openGL.capture.frames.frameSeparators.glFlushSep = glFlushSep;
    } else if (cfg.recorder.openGL.capture.mode.find("All") != std::string::npos) {
      cfg.recorder.openGL.capture.frames.startFrame = 1;
      cfg.recorder.openGL.capture.all.exitFrame = exitFrameOpenGL;
      cfg.recorder.openGL.capture.all.exitDeleteContext = exitDeleteContext;
    } else if (cfg.recorder.openGL.capture.mode.find("OglSingleDraw") != std::string::npos) {
      cfg.recorder.openGL.capture.oglSingleDraw.number = number;
    } else if (cfg.recorder.openGL.capture.mode.find("OglDrawsRange") != std::string::npos) {
      cfg.recorder.openGL.capture.oglDrawsRange.startDraw = startDraw;
      cfg.recorder.openGL.capture.oglDrawsRange.stopDraw = stopDraw;
      cfg.recorder.openGL.capture.oglDrawsRange.frame = frame;
    } else {
      Log(ERR) << "Not supported OpenGL recording mode selected.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  ReadRecorderOption(pt, "OpenGL.Utilities.TraceGLError", cfg.common.traceGLError,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.EndFrameSleep",
                     cfg.recorder.openGL.utilities.endFrameSleep, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.StripIndicesValues",
                     cfg.recorder.openGL.utilities.stripIndicesValues, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.RestoreDefaultFB",
                     cfg.recorder.openGL.utilities.restoreDefaulFB, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.DoNotRemoveWindow",
                     cfg.recorder.openGL.utilities.doNotRemoveWin, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.MultiApiProtectBypass",
                     cfg.recorder.openGL.utilities.multiApiProtectBypass, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.SuppressProgramBinary",
                     cfg.recorder.openGL.utilities.suppressProgramBinary, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.UpdateMappedTexturesEveryNSwaps",
                     cfg.recorder.openGL.utilities.updateMappedTexturesEveryNSwaps, 0);
  if (cfg.recorder.openGL.utilities.updateMappedTexturesEveryNSwaps == 0) {
    cfg.recorder.openGL.utilities.updateMappedTexturesEveryNSwaps = 1;
  }
  std::string optionValue;

  optionValue = "Mixed";
  ReadRecorderOption(pt, "OpenGL.Utilities.BuffersState", optionValue,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  cfg.recorder.openGL.utilities.buffersState.setFromString(optionValue);
  optionValue = "Mixed";
  ReadRecorderOption(pt, "OpenGL.Utilities.TexturesState", optionValue,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  cfg.recorder.openGL.utilities.texturesState.setFromString(optionValue);
  ReadRecorderOption(pt, "OpenGL.Utilities.DetectRecursion",
                     cfg.recorder.openGL.utilities.detectRecursion,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "OpenGL.Utilities.ForceGLVersion",
                     cfg.recorder.openGL.utilities.forceGLVersion, GITS_PLATFORM_BIT_ALL);
  const std::string& forceGLVersion = cfg.recorder.openGL.utilities.forceGLVersion;
  if (!forceGLVersion.empty()) {
    // The version is <major_version>.<minor_version> optionally followed by additional text.
    std::regex oglVersion("([0-9]+)\\.([0-9]+).*");
    std::smatch match;

    if (std::regex_search(forceGLVersion.begin(), forceGLVersion.end(), match, oglVersion)) {
      auto& forceGLVersionMajor = cfg.recorder.openGL.utilities.forceGLVersionMajor;
      auto& forceGLVersionMinor = cfg.recorder.openGL.utilities.forceGLVersionMinor;

      // match[0] is the whole input string (forceGLVersion).
      forceGLVersionMajor = stoui(match[1]);
      forceGLVersionMinor = stoui(match[2]);

      Log(INFO) << "OpenGL version string override set to: " << forceGLVersion;
      Log(INFO) << "OpenGL version number override set to: " << forceGLVersionMajor << "."
                << forceGLVersionMinor;
    } else {
      auto msg = "Incorrect OpenGL version string specified: " + forceGLVersion +
                 "\nCheck recorder config.";
      Log(ERR) << msg;
      throw runtime_error((std::string)EXCEPTION_MESSAGE + " " + msg);
    }
  }
  ReadRecorderOption(pt, "OpenGL.Utilities.SuppressExtensions",
                     cfg.recorder.openGL.utilities.suppressExtensions, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.CArrayMemCmpType",
                     cfg.recorder.openGL.utilities.carrayMemCmpType, GITS_PLATFORM_BIT_ALL);
  cfg.recorder.openGL.utilities.ccodeRangesWA = false;
  cfg.recorder.openGL.utilities.useGlGetTexImageAndRestoreBuffersWhenPossibleES = false;
  ReadRecorderOption(pt, "OpenGL.Utilities.ScheduleFboEXTAsCoreWA",
                     cfg.recorder.openGL.utilities.schedulefboEXTAsCoreWA,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "OpenGL.Utilities.UseGlGetTexImageAndRestoreBuffersWhenPossibleES",
                     cfg.recorder.openGL.utilities.useGlGetTexImageAndRestoreBuffersWhenPossibleES,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "OpenGL.Utilities.CCodeRangesWA",
                     cfg.recorder.openGL.utilities.ccodeRangesWA,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11, true);
  ReadRecorderOption(pt, "OpenGL.Utilities.OptimizeBufferSize",
                     cfg.recorder.openGL.utilities.optimizeBufferSize, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Utilities.CoherentMapUpdatePerFrame",
                     cfg.recorder.openGL.utilities.coherentMapUpdatePerFrame,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "OpenGL.Utilities.TrackTextureBindingWA",
                     cfg.recorder.openGL.utilities.trackTextureBindingWA,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "OpenGL.Utilities.ForceBuffersStateCaptureAlwaysWA",
                     cfg.recorder.openGL.utilities.forceBuffersStateCaptureAlwaysWA,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "OpenGL.Utilities.RestoreIndexedTexturesWA",
                     cfg.recorder.openGL.utilities.restoreIndexedTexturesWA,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "OpenGL.Utilities.RetryFunctionLoads",
                     cfg.recorder.openGL.utilities.retryFunctionLoads, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Images.DumpScreenshots",
                     cfg.recorder.openGL.images.dumpScreenshots, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Images.DumpDrawsFromFrames",
                     cfg.recorder.openGL.images.dumpDrawsFromFrames, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenGL.Performance.Benchmark", cfg.recorder.openGL.performance.benchmark,
                     GITS_PLATFORM_BIT_ALL);
  if (cfg.recorder.basic.dumpCCode == true &&
      cfg.recorder.openGL.capture.mode.find("All") == std::string::npos &&
      cfg.recorder.openGL.utilities.ccodeRangesWA == false) {
    Log(ERR) << "CCodeDump is possible only if OpenGL.Capture.Mode is set to: All. So, if you for "
                "example need one frame CCode stream please record one frame binary stream and "
                "then recapture it to CCode.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  //****************************** VULKAN ****************************************
#if defined GITS_PLATFORM_WINDOWS || defined GITS_PLATFORM_X11
  ReadRecorderOption(pt, "Vulkan.Capture.Mode", cfg.recorder.vulkan.capture.mode,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  {
    //Frames
    int startFrame = 0;
    int stopFrame = 0;
    ReadRecorderOption(pt, "Vulkan.Capture.Frames.StartFrame", startFrame,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
    ReadRecorderOption(pt, "Vulkan.Capture.Frames.StopFrame", stopFrame,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

    std::vector<std::string> startkeys;
    std::vector<unsigned> startKeyCodes;
    ReadRecorderOption(pt, "Vulkan.Capture.Frames.StartKeys", startkeys,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
    if (!startkeys.empty()) {
      std::transform(startkeys.begin(), startkeys.end(), std::back_inserter(startKeyCodes),
                     GetKeyVal);
      if (std::find(startKeyCodes.begin(), startKeyCodes.end(), 0U) != startKeyCodes.end()) {
        Log(ERR) << "Invalid Vulkan recording startup key combination given.";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }

    uint exitFrameVulkan = 0;
    std::string queueSubmitNumber;
    ReadRecorderOption(pt, "Vulkan.Capture.QueueSubmit.Number", queueSubmitNumber,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
    std::string commandBuffersRange;
    ReadRecorderOption(pt, "Vulkan.Capture.CommandBuffersRange.Range", commandBuffersRange,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
    std::string renderPassRange;
    ReadRecorderOption(pt, "Vulkan.Capture.RenderPassRange.Range", renderPassRange,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

    //init default
    cfg.recorder.vulkan.capture.frames.startFrame = static_cast<unsigned>(-1);
    cfg.recorder.vulkan.capture.frames.stopFrame = static_cast<unsigned>(-1);
    cfg.recorder.vulkan.capture.all.exitFrame = static_cast<unsigned>(-1);
    cfg.recorder.vulkan.capture.objRange.rangeSpecial.range = BitRange(false);
    cfg.recorder.vulkan.capture.objRange.rangeSpecial.objMode = MODE_VKNONE;

    ReadRecorderOption(pt, "Vulkan.Capture.All.ExitFrame", exitFrameVulkan,
                       GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
    bool foundAllMode = cfg.recorder.vulkan.capture.mode.find("All") != std::string::npos;
    bool foundFramesMode = cfg.recorder.vulkan.capture.mode.find("Frames") != std::string::npos;
    bool foundQueueSubmitMode =
        cfg.recorder.vulkan.capture.mode.find("QueueSubmit") != std::string::npos;
    bool foundCommandBuffersRangeMode =
        cfg.recorder.vulkan.capture.mode.find("CommandBuffersRange") != std::string::npos;
    bool foundRenderPassRangeMode =
        cfg.recorder.vulkan.capture.mode.find("RenderPassRange") != std::string::npos;
    if (foundAllMode) {
      cfg.recorder.vulkan.capture.frames.startFrame = 1;
      cfg.recorder.vulkan.capture.all.exitFrame = exitFrameVulkan;
    } else if (foundFramesMode) //Choose frame range basing on mode and params
    {
      cfg.recorder.vulkan.capture.frames.startFrame = startFrame;
      cfg.recorder.vulkan.capture.frames.stopFrame = stopFrame;
      cfg.recorder.vulkan.capture.frames.startKeys.swap(startKeyCodes);
    } else if (foundQueueSubmitMode || foundCommandBuffersRangeMode || foundRenderPassRangeMode) {
      std::string objectRange;
      if (foundQueueSubmitMode) {
        objectRange = queueSubmitNumber;
      } else if (foundCommandBuffersRangeMode) {
        objectRange = commandBuffersRange;
      } else if (foundRenderPassRangeMode) {
        objectRange = renderPassRange;
      }
      if (!objectRange.empty()) {
        std::istringstream issVulkanObjectsRange(objectRange);
        std::vector<std::string> objectsTable;

        std::string strObj;
        while (std::getline(issVulkanObjectsRange, strObj, '/')) {
          objectsTable.push_back(strObj);
        }
        cfg.recorder.vulkan.capture.objRange.rangeSpecial.range = BitRange(objectsTable.back());
        objectsTable.pop_back();

        for (auto obj : objectsTable) {
          cfg.recorder.vulkan.capture.objRange.rangeSpecial.objVector.push_back(
              std::stoul(obj, nullptr, 0));
        }

        if (objectsTable.size() == 0 && foundQueueSubmitMode) {
          cfg.recorder.vulkan.capture.objRange.rangeSpecial.objMode = MODE_VKQUEUESUBMIT;
        } else if (objectsTable.size() == 2 && foundCommandBuffersRangeMode) {
          cfg.recorder.vulkan.capture.objRange.rangeSpecial.objMode = MODE_VKCOMMANDBUFFER;
        } else if (objectsTable.size() == 3 && foundRenderPassRangeMode) {
          cfg.recorder.vulkan.capture.objRange.rangeSpecial.objMode = MODE_VKRENDERPASS;
        } else {
          Log(ERR) << "Incorrect range for mode: " << cfg.recorder.vulkan.capture.mode;
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
      }
    } else {
      Log(ERR) << "Not supported Vulkan recording mode selected.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  vi_bool traceVKStructs;
  ReadRecorderOption(pt, "Vulkan.Utilities.TraceVKStructs", traceVKStructs,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  if (traceVKStructs) {
    cfg.common.traceDataOpts.insert(TraceData::VK_STRUCTS);
  }
  ReadRecorderOption(pt, "Vulkan.Utilities.MemorySegmentSize",
                     cfg.recorder.vulkan.utilities.memorySegmentSize,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ShadowMemory",
                     cfg.recorder.vulkan.utilities.shadowMemory,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryAccessDetection",
                     cfg.recorder.vulkan.utilities.memoryAccessDetection,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ForceUniversalRecording",
                     cfg.recorder.vulkan.utilities.forceUniversalRecording,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.UseExternalMemoryExtension",
                     cfg.recorder.vulkan.utilities.useExternalMemoryExtension,
                     GITS_PLATFORM_BIT_WINDOWS);

  if (cfg.recorder.vulkan.utilities.useExternalMemoryExtension) {
    cfg.recorder.vulkan.utilities.shadowMemory = false;
    cfg.recorder.vulkan.utilities.memoryAccessDetection = false;
  }

  ReadRecorderOption(pt, "Vulkan.Utilities.MinimalStateRestore",
                     cfg.recorder.vulkan.utilities.minimalStateRestore,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ReusableStateRestoreResourcesCount",
                     cfg.recorder.vulkan.utilities.reusableStateRestoreResourcesCount,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ReusableStateRestoreBufferSize",
                     cfg.recorder.vulkan.utilities.reusableStateRestoreBufferSize,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  if (cfg.recorder.vulkan.utilities.reusableStateRestoreResourcesCount < 2) {
    cfg.recorder.vulkan.utilities.reusableStateRestoreResourcesCount = 2;
    Log(WARN) << "Value of vulkan.utilities.reusableStateRestoreResourcesCount should not be lower "
                 "than 2";
  }
  if (cfg.recorder.vulkan.utilities.reusableStateRestoreBufferSize < 1) {
    cfg.recorder.vulkan.utilities.reusableStateRestoreBufferSize = 1;
    Log(WARN) << "Value of vulkan.utilities.reusableStateRestoreBufferSize must be larger than 0. "
                 "Temporary buffer's size cannot be smaller than 1 MB.";
  }

  std::string vulkanOptionValue = "OnlyUsed";
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryUpdateState", vulkanOptionValue,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  cfg.recorder.vulkan.utilities.memoryUpdateState.setFromString(vulkanOptionValue);

  ReadRecorderOption(pt, "Vulkan.Utilities.DelayFenceChecksCount",
                     cfg.recorder.vulkan.utilities.delayFenceChecksCount,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ShortenFenceWaitTime",
                     cfg.recorder.vulkan.utilities.shortenFenceWaitTime,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.SuppressExtensions",
                     cfg.recorder.vulkan.utilities.suppressExtensions,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.SuppressLayers",
                     cfg.recorder.vulkan.utilities.suppressLayers,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.AddImageUsageFlags",
                     cfg.recorder.vulkan.utilities.addImageUsageFlags,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.AddBufferUsageFlags",
                     cfg.recorder.vulkan.utilities.addBufferUsageFlags,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.ScheduleCommandBuffersBeforeQueueSubmitWA",
                     cfg.recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.IncreaseImageMemorySizeRequirement.FixedAmount",
                     cfg.recorder.vulkan.utilities.increaseImageMemorySizeRequirement.fixedAmount,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.IncreaseImageMemorySizeRequirement.Percent",
                     cfg.recorder.vulkan.utilities.increaseImageMemorySizeRequirement.percent,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.SuppressPhysicalDeviceFeatures",
                     cfg.recorder.vulkan.utilities.suppressPhysicalDeviceFeatures,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryOffsetAlignmentOverride.Images",
                     cfg.recorder.vulkan.utilities.memoryOffsetAlignmentOverride.images,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryOffsetAlignmentOverride.Buffers",
                     cfg.recorder.vulkan.utilities.memoryOffsetAlignmentOverride.buffers,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryOffsetAlignmentOverride.Descriptors",
                     cfg.recorder.vulkan.utilities.memoryOffsetAlignmentOverride.descriptors,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.CrossPlatformStateRestoration.Images",
                     cfg.recorder.vulkan.utilities.crossPlatformStateRestoration.images,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  std::string vulkanBufferRestorationOptionValue = "WithNonHostVisibleMemoryOnly";
  ReadRecorderOption(pt, "Vulkan.Utilities.CrossPlatformStateRestoration.Buffers",
                     vulkanBufferRestorationOptionValue,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  cfg.recorder.vulkan.utilities.crossPlatformStateRestoration.buffers.setFromString(
      vulkanBufferRestorationOptionValue);

  std::string vulkanMemoryRestorationOptionValue = "HostVisible";
  ReadRecorderOption(pt, "Vulkan.Utilities.MemoryRestoration", vulkanMemoryRestorationOptionValue,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  cfg.recorder.vulkan.utilities.memoryRestoration.setFromString(vulkanMemoryRestorationOptionValue);
  ReadRecorderOption(pt, "Vulkan.Utilities.RestoreMultisampleImagesWA",
                     cfg.recorder.vulkan.utilities.restoreMultisampleImagesWA,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.MaxArraySizeForCCode",
                     cfg.recorder.vulkan.utilities.maxArraySizeForCCode,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(
      pt, "Vulkan.Utilities.UseCaptureReplayFeaturesForBuffersAndAccelerationStructures",
      cfg.recorder.vulkan.utilities.useCaptureReplayFeaturesForBuffersAndAccelerationStructures,
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.UseCaptureReplayFeaturesForRayTracingPipelines",
                     cfg.recorder.vulkan.utilities.useCaptureReplayFeaturesForRayTracingPipelines,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Utilities.UsePresentSrcLayoutTransitionAsAFrameBoundary",
                     cfg.recorder.vulkan.utilities.usePresentSrcLayoutTransitionAsAFrameBoundary,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "Vulkan.Utilities.RenderDocCompatibility",
                     cfg.recorder.vulkan.utilities.renderDocCompatibility,
                     GITS_PLATFORM_BIT_WINDOWS);
  if (cfg.recorder.vulkan.utilities.renderDocCompatibility) {
    auto& utilities = cfg.recorder.vulkan.utilities;
    auto& rdocSuppressExtensions = utilities.renderDocCompatibilitySuppressedExtensions;
    auto& suppressExtensions = utilities.suppressExtensions;
    std::copy_if(rdocSuppressExtensions.begin(), rdocSuppressExtensions.end(),
                 std::back_inserter(suppressExtensions), [suppressExtensions](std::string s) {
                   return std::find(suppressExtensions.begin(), suppressExtensions.end(), s) ==
                          suppressExtensions.end();
                 });
  }
  ReadRecorderOption(pt, "Vulkan.Performance.Benchmark", cfg.recorder.vulkan.performance.benchmark,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Images.DumpScreenshots",
                     cfg.recorder.vulkan.images.dumpScreenshots,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Vulkan.Images.DumpSubmits", cfg.recorder.vulkan.images.dumpSubmits,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

#endif
  std::string removeAPISharing;
  ReadRecorderOption(pt, "Extras.Utilities.RemoveAPISharing", removeAPISharing,
                     GITS_PLATFORM_BIT_ALL);
  //****************************** OPENCL ****************************************
  optionValue = "False";
  ReadRecorderOption(pt, "OpenCL.Utilities.DumpKernels", cfg.recorder.openCL.utilities.dumpKernels,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenCL.Utilities.DumpImages", cfg.recorder.openCL.utilities.dumpImages,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenCL.Utilities.OmitReadOnlyObjects",
                     cfg.recorder.openCL.utilities.omitReadOnlyObjects, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenCL.Utilities.BufferResetAfterCreate",
                     cfg.recorder.openCL.utilities.bufferResetAfterCreate, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenCL.Utilities.NullIndirectPointersInBuffer",
                     cfg.recorder.openCL.utilities.nullIndirectPointersInBuffer,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "OpenCL.Capture.Mode", cfg.recorder.openCL.capture.mode,
                     GITS_PLATFORM_BIT_ALL);
  {
    uint number = 0;
    uint startKernel = 0;
    uint stopKernel = 0;

    cfg.recorder.openCL.capture.oclSingleKernel.number = static_cast<unsigned>(-1);
    cfg.recorder.openCL.capture.oclKernelsRange.startKernel = static_cast<unsigned>(-1);
    cfg.recorder.openCL.capture.oclKernelsRange.stopKernel = static_cast<unsigned>(-1);

    ReadRecorderOption(pt, "OpenCL.Capture.OclSingleKernel.Number", number, GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenCL.Capture.OclKernelsRange.StartKernel", startKernel,
                       GITS_PLATFORM_BIT_ALL);
    ReadRecorderOption(pt, "OpenCL.Capture.OclKernelsRange.StopKernel", stopKernel,
                       GITS_PLATFORM_BIT_ALL);

    if (cfg.recorder.openCL.capture.mode.find("OclSingleKernel") != std::string::npos) {
      // WA
      cfg.recorder.openGL.capture.frames.startFrame = static_cast<unsigned>(-1);
      cfg.recorder.openCL.capture.oclSingleKernel.number = number;
    } else if (cfg.recorder.openCL.capture.mode.find("OclKernelsRange") != std::string::npos) {
      cfg.recorder.openGL.capture.frames.startFrame = static_cast<unsigned>(-1);
      cfg.recorder.openCL.capture.oclKernelsRange.startKernel = startKernel;
      cfg.recorder.openCL.capture.oclKernelsRange.stopKernel = stopKernel;
    }
  }
  if (cfg.recorder.openCL.capture.mode.find("All") == std::string::npos &&
      !removeAPISharing.empty()) {
    Log(ERR) << "Unsharing is not supported in subcapture mode. If you need a subcapture from "
                "sharing stream, create unshared stream first, then do a subcapture of that.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  //****************************** LevelZero ****************************************
#ifdef WITH_LEVELZERO
  std::string kernelInfo;
  ReadRecorderOption(pt, "LevelZero.Capture.Mode", cfg.recorder.levelZero.capture.mode,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "LevelZero.Utilities.BufferResetAfterCreate",
                     cfg.recorder.levelZero.utilities.bufferResetAfterCreate,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "LevelZero.Utilities.NullIndirectPointersInBuffer",
                     cfg.recorder.levelZero.utilities.nullIndirectPointersInBuffer,
                     GITS_PLATFORM_BIT_ALL);
  {
    if (cfg.recorder.levelZero.capture.mode.find("Kernel") != std::string::npos) {
      ReadRecorderOption(pt, "LevelZero.Capture.Kernel.Range", kernelInfo, GITS_PLATFORM_BIT_ALL);
      if (!kernelInfo.empty()) {
        // WA (trigger for _captureState to be CAP_INITIATED for delaying recorder.Start())
        cfg.recorder.openGL.capture.frames.startFrame = static_cast<unsigned>(-1);
        std::istringstream issL0ObjectsRange(kernelInfo);
        std::vector<std::string> objectsTable;
        std::string strObj;
        while (std::getline(issL0ObjectsRange, strObj, '/')) {
          objectsTable.push_back(strObj);
        }
        if (objectsTable.size() != 3) {
          Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range";
          throw EOperationFailed(EXCEPTION_MESSAGE);
        }
        cfg.recorder.levelZero.capture.kernel.queueRange = BitRange(objectsTable[0]);
        auto& maxQueueSubmitNumber = cfg.recorder.levelZero.capture.kernel.maxQueueSubmitNumber;
        auto& minQueueSubmitNumber = cfg.recorder.levelZero.capture.kernel.minQueueSubmitNumber;
        minQueueSubmitNumber = std::numeric_limits<unsigned>::max();
        maxQueueSubmitNumber = 0;
        auto pattern = std::regex("(\\d+)");
        auto iter = std::sregex_iterator(objectsTable[0].begin(), objectsTable[0].end(), pattern);
        auto end = std::sregex_iterator();
        while (iter != end) {
          minQueueSubmitNumber =
              std::min((unsigned)minQueueSubmitNumber, (unsigned)std::stoul((*iter)[0], nullptr));
          maxQueueSubmitNumber =
              std::max((unsigned)maxQueueSubmitNumber, (unsigned)std::stoul((*iter++)[0], nullptr));
        }
        cfg.recorder.levelZero.capture.kernel.cmdListRange = BitRange(objectsTable[1]);
        cfg.recorder.levelZero.capture.kernel.kernelRange = BitRange(objectsTable[2]);
        auto& startKernel = cfg.recorder.levelZero.capture.kernel.startKernel;
        auto& stopKernel = cfg.recorder.levelZero.capture.kernel.stopKernel;
        startKernel = 0;
        iter = std::sregex_iterator(objectsTable[2].begin(), objectsTable[2].end(), pattern);
        while (iter != end) {
          stopKernel = std::max((unsigned)stopKernel, (unsigned)std::stoul((*iter)[0], nullptr));
          startKernel =
              std::min((unsigned)startKernel, (unsigned)std::stoul((*iter++)[0], nullptr));
        }
        if (startKernel == 0) {
          startKernel = stopKernel;
        }
        pattern = std::regex(R"(^\d/\d/\d$)");
        iter = std::sregex_iterator(kernelInfo.begin(), kernelInfo.end(), pattern);
        cfg.recorder.levelZero.capture.kernel.singleCapture = (iter != end);
      }
    }
    {
      std::string kernelDumpInfo;
      ReadRecorderOption(pt, "LevelZero.Utilities.DumpKernels", kernelDumpInfo,
                         GITS_PLATFORM_BIT_ALL);
      std::vector<std::string> objectsTable;
      std::istringstream issL0ObjectsRange(kernelDumpInfo);
      std::string strObj;
      if (kernelDumpInfo.find_first_of('/') != std::string::npos) {
        while (std::getline(issL0ObjectsRange, strObj, '/')) {
          objectsTable.push_back(strObj);
        }
      } else {
        while (std::getline(issL0ObjectsRange, strObj, '\\')) {
          objectsTable.push_back(strObj);
        }
      }
      if (objectsTable.size() == 2) {
        cfg.recorder.levelZero.utilities.captureCommandQueues = BitRange(objectsTable[0]);
        cfg.recorder.levelZero.utilities.captureCommandLists = BitRange(objectsTable[1]);
        cfg.recorder.levelZero.utilities.captureKernels = BitRange(true);
      } else if (objectsTable.size() == 3) {
        cfg.recorder.levelZero.utilities.captureCommandQueues = BitRange(objectsTable[0]);
        cfg.recorder.levelZero.utilities.captureCommandLists = BitRange(objectsTable[1]);
        cfg.recorder.levelZero.utilities.captureKernels = BitRange(objectsTable[2]);
      } else {
        Log(ERR) << "Incorrect config LevelZero.Utilities.DumpKernels";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }
  }
  ReadRecorderOption(pt, "LevelZero.Utilities.DumpAfterSubmit",
                     cfg.recorder.levelZero.utilities.captureAfterSubmit, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "LevelZero.Utilities.DumpImages",
                     cfg.recorder.levelZero.utilities.captureImages, GITS_PLATFORM_BIT_ALL);
#endif
  //******************************** EXTRAS ******************************************
  ReadRecorderOption(pt, "Extras.Optimizations.TokenBurstLimit", cfg.common.tokenBurst,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.HashType", optionValue, GITS_PLATFORM_BIT_ALL);
  cfg.recorder.extras.optimizations.hashType.setFromString(optionValue);
  ReadRecorderOption(pt, "Extras.Optimizations.HashPartially",
                     cfg.recorder.extras.optimizations.partialHash.enabled, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.PartialHashCutoff",
                     cfg.recorder.extras.optimizations.partialHash.cutoff, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.PartialHashChunks",
                     cfg.recorder.extras.optimizations.partialHash.chunks, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.PartialHashRatio",
                     cfg.recorder.extras.optimizations.partialHash.ratio, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.AsyncBufferWrites",
                     cfg.recorder.extras.optimizations.asyncBufferWrites, GITS_PLATFORM_BIT_ALL);
  cfg.recorder.extras.optimizations.bufferMapAccessMask = 0xFFFFFFFF;
  ReadRecorderOption(pt, "Extras.Optimizations.BufferMapAccessMask",
                     cfg.recorder.extras.optimizations.bufferMapAccessMask, GITS_PLATFORM_BIT_ALL);
  cfg.recorder.extras.optimizations.bufferStorageFlagsMask = 0;
  ReadRecorderOption(pt, "Extras.Optimizations.BufferStorageFlagsMask",
                     cfg.recorder.extras.optimizations.bufferStorageFlagsMask,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Optimizations.RemoveResourceHash",
                     cfg.recorder.extras.optimizations.removeResourceHash, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Utilities.ForceDumpOnError",
                     cfg.recorder.extras.utilities.forceDumpOnError, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Utilities.ExtendedDiagnostic",
                     cfg.recorder.extras.utilities.extendedDiagnosticInfo, GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Utilities.NullIO", cfg.recorder.extras.utilities.nullIO,
                     GITS_PLATFORM_BIT_ALL);
  ReadRecorderOption(pt, "Extras.Utilities.MTDriverWA", cfg.recorder.extras.utilities.mtDriverWA,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "Extras.Utilities.CoherentMapBehaviorWA",
                     cfg.recorder.extras.utilities.coherentMapBehaviorWA,
                     GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  ReadRecorderOption(pt, "Extras.Utilities.CloseAppOnStopRecording",
                     cfg.recorder.extras.utilities.closeAppOnStopRecording,
                     GITS_PLATFORM_BIT_WINDOWS);
  ReadRecorderOption(pt, "Extras.Utilities.ZipTextFiles",
                     cfg.recorder.extras.utilities.zipTextFiles, GITS_PLATFORM_BIT_ALL);
  if (cfg.recorder.basic.dumpCCode) {
    cfg.recorder.extras.utilities.zipTextFiles = false;
  }
  ReadRecorderOption(pt, "Extras.Utilities.HighIntegrity",
                     cfg.recorder.extras.utilities.highIntegrity, GITS_PLATFORM_BIT_ALL);
  if (cfg.recorder.extras.utilities.highIntegrity) {
    Log(INFO) << "High integrity mode";
    cfg.recorder.extras.optimizations.asyncBufferWrites = 0;
    cfg.common.tokenBurst = 1;
    cfg.common.tokenBurstNum = 1;
    if (cfg.recorder.extras.utilities.zipTextFiles) {
      cfg.recorder.extras.utilities.zipTextFiles = false;
      Log(WARN) << "High Integrity mode active - overriding ZipTextFiles to False.";
    }
  }
#if !defined(BUILD_FOR_CCODE)
  std::string eventScript;
  ReadRecorderOption(pt, "Extras.Utilities.EventScript", eventScript, GITS_PLATFORM_BIT_ALL);
  if (!eventScript.empty()) {
    boost::filesystem::path scriptPath(eventScript);
    if (!exists(scriptPath) || !is_regular_file(scriptPath)) {
      throw std::runtime_error("could not find file: " + eventScript);
    }

    lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    CGits::Instance().ProcessLuaFunctionsRegistrators();
    cfg.common.useEvents = true;
  }
#endif
  {
    auto& removeDXSharing = cfg.recorder.extras.utilities.removeDXSharing;
    auto& removeGLSharing = cfg.recorder.extras.utilities.removeGLSharing;
    if (!removeAPISharing.empty()) {
      std::smatch match;
      auto pattern = std::regex("[a|A][l|L]{2}");
      if (std::regex_search(removeAPISharing, match, pattern)) {
        removeDXSharing = true;
        removeGLSharing = true;
      } else {
        pattern = std::regex("[d|D][x|X]");
        removeDXSharing = std::regex_search(removeAPISharing, match, pattern);
        pattern = std::regex("[g|G][l|L]");
        removeGLSharing = std::regex_search(removeAPISharing, match, pattern);
      }
    }
  }
  optionValue = "MessageLoop";
  ReadRecorderOption(pt, "Extras.Utilities.WindowsKeyHandling", optionValue,
                     GITS_PLATFORM_BIT_WINDOWS);
  cfg.recorder.extras.utilities.windowsKeyHandling.setFromString(optionValue);

  Set(cfg);

  {
    // create file data and register it in GITS
    CGits& inst = CGits::Instance();
    std::unique_ptr<CFile> file(new CFile(inst.Version()));

    gather_diagnostic_info(file->GetPropertyTree());
    file->GetPropertyTree().add("diag.gits.config_path", cfgPath);
    file->GetPropertyTree().add("diag.gits.config", read_file(cfgPath));
    inst.Register(std::move(file));
  }

  configured = true;
  return true;
}

void gits::GetConfigPtree(const boost::filesystem::path& cfgPath, boost::property_tree::ptree& pt) {
  auto string = cfgPath.string();
  std::ifstream cfgFile(string.c_str());
  if (!cfgFile.good()) {
    throw std::runtime_error("Error: Config file not found.");
  }

  // check for BOM and if present discard it
  char bom[3];
  cfgFile.read(bom, 3);
  if (!cfgFile) {
    throw std::runtime_error("Error: Could not inspect config file for BOM presence.");
  }
  if (!(bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF')) {
    cfgFile.close();
    cfgFile.open(string.c_str());
    if (!cfgFile.good()) {
      throw std::runtime_error("Error: Config file not found.");
    }
  }
  read_info(cfgFile, pt);
  if (cfgFile.is_open()) {
    cfgFile.close();
  }
}

template <>
gits::CEnumParser<gits::LogLevel>::CEnumParser() {
  if (_map.empty()) {
    _map["trace verbose"] = TRACEV;
    _map["traceverbose"] = TRACEV;
    _map["tracev"] = TRACEV;
    _map["trace"] = TRACE;
    _map["infov"] = INFOV;
    _map["info verbose"] = INFOV;
    _map["infoverbose"] = INFOV;
    _map["info"] = INFO;
    _map["warning"] = WARN;
    _map["warn"] = WARN;
    _map["error"] = ERR;
    _map["err"] = ERR;
    _map["off"] = OFF;
  }
}

template <>
void gits::ReadRecorderOption<std::string>(const boost::property_tree::ptree& pt,
                                           const char* name,
                                           std::string& value,
                                           unsigned supportedPlatforms,
                                           bool hide) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    //Remove ",' from string begin and string end
    value = *optVal;
    if (value.size() < 2) {
      return;
    }
    if (*value.begin() == '"' || *value.begin() == '\'') {
      value.erase(value.begin());
    }
    if (*value.rbegin() == '"' || *value.rbegin() == '\'') {
      value.erase(--value.end());
    }
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

template <>
void gits::ReadRecorderOption<boost::filesystem::path>(const boost::property_tree::ptree& pt,
                                                       const char* name,
                                                       boost::filesystem::path& value,
                                                       unsigned supportedPlatforms,
                                                       bool hide) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    std::string str = *optVal;
    ReadRecorderOption(pt, name, str, supportedPlatforms);
    value = str;
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

template <>
void gits::ReadRecorderOption<gits::vi_uint>(const boost::property_tree::ptree& pt,
                                             const char* name,
                                             gits::vi_uint& value,
                                             unsigned supportedPlatforms,
                                             bool hide) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    std::stringstream sstream(*optVal);
    if ((*optVal).compare(0, 2, "0x") == 0) {
      sstream << std::hex;
    }
    sstream >> value;
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

template <>
void gits::ReadRecorderOption<gits::vi_bool>(const boost::property_tree::ptree& pt,
                                             const char* name,
                                             gits::vi_bool& value,
                                             unsigned supportedPlatforms,
                                             bool hide) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    if (*optVal == "True") {
      value = true;
    } else if (*optVal == "False") {
      value = false;
    } else {
      Log(ERR) << "Option: " << name << " has incorrect value in config file.";
      throw std::runtime_error("Config parser error.");
    }
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

template <>
void gits::ReadRecorderOption<BitRange>(const boost::property_tree::ptree& pt,
                                        const char* name,
                                        BitRange& value,
                                        unsigned supportedPlatforms,
                                        bool hide) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    value = BitRange(*optVal);
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

template <>
void gits::ReadRecorderOption<std::vector<string>>(const boost::property_tree::ptree& pt,
                                                   const char* name,
                                                   std::vector<string>& value,
                                                   unsigned supportedPlatforms,
                                                   bool hide) {
  boost::optional<std::string> optVal = pt.get_optional<std::string>(name);
  if (optVal) {
    value.clear();
    std::stringstream sstream(*optVal);
    std::string elem;
    while (std::getline(sstream, elem, ',')) {
      value.push_back(elem);
    }
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}

bool gits::Config::IsRecorder() {
  return Get().common.mode == MODE_RECORDER;
}

bool gits::Config::IsPlayer() {
  return Get().common.mode == MODE_PLAYER;
}

bool gits::isTraceDataOptPresent(gits::TraceData option) {
  auto& traceDataOpts = Config::Get().common.traceDataOpts;
  return traceDataOpts.find(option) != traceDataOpts.end();
}

#else
// Ccode only implementation.
bool gits::Config::IsRecorder() {
  return false;
}

bool gits::Config::IsPlayer() {
  return true;
}

bool gits::isTraceDataOptPresent(gits::TraceData option) {
  return false;
}

#endif

bool gits::Config::VulkanObjectRange::operator[](uint64_t queueSubmitNumber) const {
  if (objMode == MODE_VKQUEUESUBMIT) {
    return range[(size_t)queueSubmitNumber];
  } else if (objMode == MODE_VKCOMMANDBUFFER || objMode == MODE_VKRENDERPASS) {
    return objVector[0] == queueSubmitNumber;
  } else {
    return false;
  }
}
