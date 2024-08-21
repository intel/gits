// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "config.h"
#include "keyEvents.h"

#ifndef BUILD_FOR_CCODE
#include "diagnostic.h"
#include "lua_bindings.h"
#include "configYamlTemplates.h"
#endif

#include <regex>
#include <chrono>
#include <filesystem>
#include <fstream>

#ifdef GITS_PLATFORM_WINDOWS
#include <process.h>
#define getpid _getpid
#endif

gits::Config* gits::Config::config = new Config;
const std::string gits::Config::CONFIG_FILE_NAME = "gits_config.yml";

gits::Config::Config() {
  common.mode = MODE_UNKNOWN;
  common.shared.thresholdLogLevel = LogLevel::INFO;
  common.player.tokenBurst = 10000;
  common.player.tokenBurstNum = 5;
  common.recorder.tokenBurst = 10000;
  common.recorder.tokenBurstNum = 5;
  common.player.exitFrame = 1000000;
  common.recorder.extendedDiagnosticInfo = true;
#ifdef GITS_PLATFORM_WINDOWS
  common.recorder.closeAppOnStopRecording = true;
#else
  common.recorder.exitSignal = 15;
#endif

  opengl.recorder.all.exitFrame = 100000000;
  opengl.player.keepDraws = BitRange(true);
  opengl.player.keepFrames = BitRange(true);
  opengl.player.scaleFactor = 1.0f;
  opengl.recorder.stripIndicesValues = 0xFFFFFFFF;
  opengl.recorder.restoreDefaultFB = true;
  opengl.recorder.suppressProgramBinary = true;
  opengl.recorder.retryFunctionLoads = true;

  vulkan.recorder.reusableStateRestoreResourcesCount = 3;
  vulkan.recorder.reusableStateRestoreBufferSize = 80;
  vulkan.recorder.maxArraySizeForCCode = 400;
  vulkan.player.maxAllowedVkSwapchainRewinds = 100;

#if defined GITS_PLATFORM_WINDOWS
  common.shared.libGL = "OpenGL32.dll";
  common.shared.libEGL = "libEGL.dll";
  common.shared.libGLESv1 = "libGLESv1_CM.dll";
  common.shared.libGLESv2 = "libGLESv2.dll";
  common.shared.libClPath = "OpenCL.dll";
  common.shared.libVK = "vulkan-1.dll";
  common.shared.libOcloc = "ocloc64.dll";
  common.shared.libL0Driver = "ze_intel_gpu64.dll";
  common.shared.libL0 = "ze_loader.dll";
#elif defined GITS_PLATFORM_X11
  common.shared.libGL = "libGL.so.1";
  common.shared.libEGL = "libEGL.so.1";
  common.shared.libGLESv1 = "libGLESv1_CM.so.1";
  common.shared.libGLESv2 = "libGLESv2.so.2";
  common.shared.libClPath = "libOpenCL.so.1";
  common.shared.libVK = "libvulkan.so.1";
  common.shared.libOcloc = "libocloc.so";
  common.shared.libL0Driver = "libze_intel_gpu.so.1";
  common.shared.libL0 = "libze_loader.so.1";
#else
#error Invalid platform.
#endif
}

std::filesystem::path gits::Config::GetConfigPath(const std::filesystem::path& appDir) {
  auto localConfigPath = appDir / CONFIG_FILE_NAME;
  if (std::filesystem::exists(localConfigPath)) {
    Log(INFO) << "Using a local config file located in: " << localConfigPath;
    return localConfigPath;
  }

  const char* envConfigPath = getenv("GITS_CONFIG_DIR");
  if (!envConfigPath) {
    return std::filesystem::path{};
  }

  auto globalConfigPath = std::filesystem::path(envConfigPath) / CONFIG_FILE_NAME;
  if (std::filesystem::exists(globalConfigPath)) {
    Log(INFO) << "Using a global config file located in: " << globalConfigPath;
    return globalConfigPath;
  }
  return std::filesystem::path{};
}

#ifndef BUILD_FOR_CCODE
YAML::Node gits::Config::LoadConfigFile(const std::filesystem::path& cfgPath) {
  YAML::Node configYaml = YAML::LoadFile(cfgPath.string());
  if (!configYaml) {
    throw std::runtime_error("Config file did not load correctly");
  }
  return configYaml;
}

void gits::Config::SetCommon(const YAML::Node& commonYaml) {
  // Shared
  common.shared = commonYaml["Shared"].as<Config::Common::Shared>();
  CLog::SetLogLevel(common.shared.thresholdLogLevel);
  gits::vi_bool logToConsoleTmp;
  if (getenv("GITS_LOG_CONSOLE") != nullptr) {
    logToConsoleTmp = true;
  }
  if (!logToConsoleTmp) {
    logToConsoleTmp = common.shared.logToConsole;
  }
  CLog::SetLogToConsole(logToConsoleTmp);

  // Player
  auto& cfgPlayer = common.player;
  cfgPlayer = commonYaml["Player"].as<Config::Common::Player>();
  if (!cfgPlayer.streamPath.empty()) {
    cfgPlayer.streamDir = cfgPlayer.streamPath.parent_path();
  }
  if (cfgPlayer.fullscreen) {
    cfgPlayer.windowMode = WindowMode::EXCLUSIVE_FULLSCREEN;
  }
  if (!cfgPlayer.eventScript.empty()) {
    auto& scriptPath = cfgPlayer.eventScript;
    if (!std::filesystem::exists(scriptPath) || !std::filesystem::is_regular_file(scriptPath)) {
      throw std::runtime_error("EventScript error: Could not find file: " + scriptPath.string());
    }
    lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    cfgPlayer.useEvents = true;
  }

  // Recorder
  auto& cfgRecorder = common.recorder;
  cfgRecorder = commonYaml["Recorder"].as<Config::Common::Recorder>();
  if (cfgRecorder.recordingMode != TRecordingMode::NONE) {
    cfgRecorder.enabled = true;
  }
  if (!cfgRecorder.exitKeysStr.empty()) {
    cfgRecorder.exitKeys = std::move(parseKeys(cfgRecorder.exitKeysStr));
  }
  cfgRecorder.dumpPath =
      cfgRecorder.PrepareDumpPath(cfgRecorder.dumpPath.string(), cfgRecorder.uniqueDumpDirectory);

  if (cfgRecorder.recordingMode == TRecordingMode::CCODE) {
    cfgRecorder.zipTextFiles = false;
  }
  if (cfgRecorder.highIntegrity) {
    Log(INFO) << "High integrity mode";
    cfgRecorder.tokenBurst = 1;
    cfgRecorder.tokenBurstNum = 1;
    if (cfgRecorder.zipTextFiles) {
      cfgRecorder.zipTextFiles = false;
      Log(WARN) << "High Integrity mode active - overriding ZipTextFiles to False.";
    }
    cfgRecorder.compression.type.setFromString("None");
    Log(WARN) << "High Integrity mode active - disabling compression.";
  }
  if (!cfgRecorder.eventScript.empty()) {
    auto& scriptPath = cfgRecorder.eventScript;
    if (!std::filesystem::exists(scriptPath) || !std::filesystem::is_regular_file(scriptPath)) {
      throw std::runtime_error("EventScript error: Could not find file: " + scriptPath.string());
    }
    lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    CGits::Instance().ProcessLuaFunctionsRegistrators();
    cfgRecorder.useEvents = true;
  }

  if (common.mode == TMode::MODE_PLAYER) {
    common.shared.libGL = common.player.libGL;
    common.shared.libEGL = common.player.libEGL;
    common.shared.libGLESv1 = common.player.libGLESv1;
    common.shared.libGLESv2 = common.player.libGLESv2;
    common.shared.libClPath = common.player.libClPath;
    common.shared.libVK = common.player.libVK;
    common.shared.libOcloc = common.player.libOcloc;
    common.shared.libL0 = common.player.libL0;
    common.shared.useEvents = common.player.useEvents;
    common.shared.scriptArgsStr = common.player.scriptArgsStr;

  } else if (common.mode == TMode::MODE_RECORDER) {
    common.shared.libGL = common.recorder.libGL;
    common.shared.libEGL = common.recorder.libEGL;
    common.shared.libGLESv1 = common.recorder.libGLESv1;
    common.shared.libGLESv2 = common.recorder.libGLESv2;
    common.shared.libClPath = common.recorder.libClPath;
    common.shared.libVK = common.recorder.libVK;
    common.shared.libOcloc = common.recorder.libOcloc;
    common.shared.libL0 = common.recorder.libL0;
    common.shared.useEvents = common.recorder.useEvents;
    common.shared.scriptArgsStr = common.recorder.scriptArgsStr;
  }
}

void gits::Config::SetOpenGL(const YAML::Node& openglYaml) {
  // Shared
  auto& cfgOpenGLShared = opengl.shared;
  cfgOpenGLShared = openglYaml["Shared"].as<Config::OpenGL::Shared>();
  if (!cfgOpenGLShared.forceGLVersion.empty()) {
    // The version is <major_version>.<minor_version> optionally followed by additional text.
    std::regex oglVersion("([0-9]+)\\.([0-9]+).*");
    std::smatch match;

    if (std::regex_search(cfgOpenGLShared.forceGLVersion, match, oglVersion)) {
      auto& forceGLVersionMajor = cfgOpenGLShared.forceGLVersionMajor;
      auto& forceGLVersionMinor = cfgOpenGLShared.forceGLVersionMinor;

      // match[0] is the whole input string (forceGLVersion).
      forceGLVersionMajor = stoui(match[1]);
      forceGLVersionMinor = stoui(match[2]);

      Log(INFO) << "OpenGL version string override set to: " << cfgOpenGLShared.forceGLVersion;
      Log(INFO) << "OpenGL version number override set to: " << forceGLVersionMajor << "."
                << forceGLVersionMinor;
    } else {
      auto msg = "Incorrect OpenGL version string specified: " + cfgOpenGLShared.forceGLVersion +
                 "\nCheck recorder config.";
      Log(ERR) << msg;
      throw std::runtime_error((std::string)EXCEPTION_MESSAGE + " " + msg);
    }
  }

  // Player
  opengl.player = openglYaml["Player"].as<Config::OpenGL::Player>();
  if (!opengl.player.affectedViewport.empty()) {
    opengl.player.affectViewport = true;
  }

  // Recorder
  auto& cfgOpenGLRecorder = opengl.recorder;
  cfgOpenGLRecorder = openglYaml["Recorder"].as<Config::OpenGL::Recorder>();
  if (cfgOpenGLRecorder.mode == TOpenGLRecorderMode::ALL) {
    cfgOpenGLRecorder.frames.startFrame = 1;
    cfgOpenGLRecorder.frames.stopFrame = UINT_MAX;
    cfgOpenGLRecorder.frames.frameSeparators.glFinishSep = false;
    cfgOpenGLRecorder.frames.frameSeparators.glFlushSep = false;
    cfgOpenGLRecorder.oglSingleDraw.number = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.startDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.stopDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.frame = UINT_MAX;
  } else if (cfgOpenGLRecorder.mode == TOpenGLRecorderMode::FRAMES) {
    cfgOpenGLRecorder.all.exitFrame = UINT_MAX;
    cfgOpenGLRecorder.all.exitDeleteContext = 0;
    cfgOpenGLRecorder.oglSingleDraw.number = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.startDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.stopDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.frame = UINT_MAX;
    if (!cfgOpenGLRecorder.frames.startKeysStr.empty()) {
      cfgOpenGLRecorder.frames.startKeys =
          std::move(parseKeys(cfgOpenGLRecorder.frames.startKeysStr));
    }
  } else if (cfgOpenGLRecorder.mode == TOpenGLRecorderMode::SINGLE_DRAW) {
    cfgOpenGLRecorder.all.exitFrame = UINT_MAX;
    cfgOpenGLRecorder.all.exitDeleteContext = 0;
    cfgOpenGLRecorder.frames.startFrame = UINT_MAX;
    cfgOpenGLRecorder.frames.stopFrame = UINT_MAX;
    cfgOpenGLRecorder.frames.frameSeparators.glFinishSep = false;
    cfgOpenGLRecorder.frames.frameSeparators.glFlushSep = false;
    cfgOpenGLRecorder.oglDrawsRange.startDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.stopDraw = UINT_MAX;
    cfgOpenGLRecorder.oglDrawsRange.frame = UINT_MAX;
  } else if (cfgOpenGLRecorder.mode == TOpenGLRecorderMode::DRAWS_RANGE) {
    cfgOpenGLRecorder.all.exitFrame = UINT_MAX;
    cfgOpenGLRecorder.all.exitDeleteContext = 0;
    cfgOpenGLRecorder.frames.startFrame = UINT_MAX;
    cfgOpenGLRecorder.frames.stopFrame = UINT_MAX;
    cfgOpenGLRecorder.frames.frameSeparators.glFinishSep = false;
    cfgOpenGLRecorder.frames.frameSeparators.glFlushSep = false;
    cfgOpenGLRecorder.oglSingleDraw.number = UINT_MAX;
  }

  if (common.recorder.recordingMode == TRecordingMode::CCODE &&
      cfgOpenGLRecorder.mode != TOpenGLRecorderMode::ALL && !cfgOpenGLRecorder.ccodeRangesWA) {
    Log(ERR) << "CCodeDump is possible only if OpenGL.Capture.Mode is set to: All. So, if you for "
                "example need one frame CCode stream please record one frame binary stream and "
                "then recapture it to CCode.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void gits::Config::SetVulkan(const YAML::Node& vulkanYaml) {
  // Shared
  vulkan.shared = vulkanYaml["Shared"].as<Config::Vulkan::Shared>();

  // Player
  vulkan.player = vulkanYaml["Player"].as<Config::Vulkan::Player>();

  // Recorder
  auto& cfgVkRecorder = vulkan.recorder;
  cfgVkRecorder = vulkanYaml["Recorder"].as<Config::Vulkan::Recorder>();

  if (cfgVkRecorder.mode == TVulkanRecorderMode::ALL) {
    cfgVkRecorder.frames.startFrame = 1;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.objRange.rangeSpecial.range = BitRange(false);
    cfgVkRecorder.objRange.rangeSpecial.objMode = VulkanObjectMode::MODE_VK_NONE;
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::FRAMES) {
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.objRange.rangeSpecial.range = BitRange(false);
    cfgVkRecorder.objRange.rangeSpecial.objMode = VulkanObjectMode::MODE_VK_NONE;
    if (!cfgVkRecorder.frames.startKeysStr.empty()) {
      cfgVkRecorder.frames.startKeys = std::move(parseKeys(cfgVkRecorder.frames.startKeysStr));
    }
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::QUEUE_SUBMIT) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.queueSubmitStr,
                                  VulkanObjectMode::MODE_VK_QUEUE_SUBMIT, 0);
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::COMMAND_BUFFERS_RANGE) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.commandBuffersRangeStr,
                                  VulkanObjectMode::MODE_VK_COMMAND_BUFFER, 2);
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::RENDER_PASS_RANGE) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.renderPassRangeStr,
                                  VulkanObjectMode::MODE_VK_RENDER_PASS, 3);
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::DRAWS_RANGE) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.drawsRangeStr, VulkanObjectMode::MODE_VK_DRAW, 4);
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::DISPATCH_RANGE) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.dispatchRangeStr,
                                  VulkanObjectMode::MODE_VK_DISPATCH, 3);
  } else if (cfgVkRecorder.mode == TVulkanRecorderMode::BLIT_RANGE) {
    cfgVkRecorder.frames.startFrame = UINT_MAX;
    cfgVkRecorder.frames.stopFrame = UINT_MAX;
    cfgVkRecorder.all.exitFrame = UINT_MAX;
    cfgVkRecorder.SetRangeSpecial(cfgVkRecorder.blitRangeStr, VulkanObjectMode::MODE_VK_BLIT, 3);
  }

  if (cfgVkRecorder.traceVkStructs) {
    common.shared.traceDataOpts.insert(TraceData::VK_STRUCTS);
  }

#ifdef GITS_PLATFORM_WINDOWS
  if (cfgVkRecorder.useExternalMemoryExtension) {
    cfgVkRecorder.shadowMemory = false;
    cfgVkRecorder.memoryAccessDetection = false;
  }
  if (cfgVkRecorder.renderDocCompatibility) {
    auto& rdocSuppressExtensions = cfgVkRecorder.renderDocCompatibilitySuppressedExtensions;
    auto& suppressExtensions = vulkan.shared.suppressExtensions;
    std::copy_if(rdocSuppressExtensions.begin(), rdocSuppressExtensions.end(),
                 std::back_inserter(suppressExtensions), [suppressExtensions](std::string s) {
                   return std::find(suppressExtensions.begin(), suppressExtensions.end(), s) ==
                          suppressExtensions.end();
                 });
  }
#endif
}

void gits::Config::SetOpenCL(const YAML::Node& openclYaml) {
  // Player
  opencl.player = openclYaml["Player"].as<Config::OpenCL::Player>();

  // Recorder
  auto& cfgOpenCLRecorder = opencl.recorder;
  cfgOpenCLRecorder = openclYaml["Recorder"].as<Config::OpenCL::Recorder>();
  if (cfgOpenCLRecorder.mode == TOpenCLRecorderMode::SINGLE_KERNEL) {
    // WA
    opengl.recorder.frames.startFrame = UINT_MAX;
    cfgOpenCLRecorder.oclKernelsRange.startKernel = UINT_MAX;
    cfgOpenCLRecorder.oclKernelsRange.stopKernel = UINT_MAX;
  } else if (cfgOpenCLRecorder.mode == TOpenCLRecorderMode::KERNELS_RANGE) {
    opengl.recorder.frames.startFrame = UINT_MAX;
    cfgOpenCLRecorder.oclSingleKernel.number = UINT_MAX;
  }
  if (cfgOpenCLRecorder.mode != TOpenCLRecorderMode::ALL &&
      (common.recorder.removeGLSharing || common.recorder.removeDXSharing)) {
    Log(ERR) << "Unsharing is not supported in subcapture mode. If you need a subcapture from "
                "sharing stream, create unshared stream first, then do a subcapture of that.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void gits::Config::SetLevelZero(const YAML::Node& levelzeroYaml) {
  // Player
  auto& cfgLevelZeroPlayer = levelzero.player;
  cfgLevelZeroPlayer = levelzeroYaml["Player"].as<Config::LevelZero::Player>();
  auto captureKernelsOpt = levelzero.ParseCaptureKernels(cfgLevelZeroPlayer.captureKernelsStr);
  cfgLevelZeroPlayer.captureCommandQueues = BitRange(captureKernelsOpt[0]);
  cfgLevelZeroPlayer.captureCommandLists = BitRange(captureKernelsOpt[1]);
  if (captureKernelsOpt.size() == 2) {
    cfgLevelZeroPlayer.captureKernels = BitRange(true);
  } else if (captureKernelsOpt.size() == 3) {
    cfgLevelZeroPlayer.captureKernels = BitRange(captureKernelsOpt[2]);
  } else {
    Log(ERR) << "Incorrect config LevelZero.Player.CaptureKernels";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  if (cfgLevelZeroPlayer.captureInputKernels && cfgLevelZeroPlayer.captureAfterSubmit) {
    Log(ERR) << "CaptureInputKernels and CaptureAfterSubmit options are mutually exclusive.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // Recorder
  auto& cfgLevelZeroRecorder = levelzero.recorder;
  cfgLevelZeroRecorder = levelzeroYaml["Recorder"].as<Config::LevelZero::Recorder>();
  if (cfgLevelZeroRecorder.mode == TLevelZeroRecorderMode::KERNEL) {
    LoadLevelZeroSubcaptureSettings(cfgLevelZeroRecorder.kernelRangeStr);
  }
  captureKernelsOpt = levelzero.ParseCaptureKernels(cfgLevelZeroRecorder.dumpKernelsStr);
  cfgLevelZeroRecorder.captureCommandQueues = BitRange(captureKernelsOpt[0]);
  cfgLevelZeroRecorder.captureCommandLists = BitRange(captureKernelsOpt[1]);
  if (captureKernelsOpt.size() == 2) {
    cfgLevelZeroRecorder.captureKernels = BitRange(true);
  } else if (captureKernelsOpt.size() == 3) {
    cfgLevelZeroRecorder.captureKernels = BitRange(captureKernelsOpt[2]);
  } else {
    Log(ERR) << "Incorrect config LevelZero.Recorder.DumpKernels";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  if (cfgLevelZeroRecorder.captureAfterSubmit && cfgLevelZeroRecorder.dumpInputKernels) {
    Log(ERR) << "DumpAfterSubmit and DumpInputKernels are mutually exclusive.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void gits::Config::LoadLevelZeroSubcaptureSettings(const std::string& kernelInfo) {
  if (!kernelInfo.empty()) {
    // WA (trigger for _captureState to be CAP_INITIATED for delaying recorder.Start())
    opengl.recorder.frames.startFrame = UINT_MAX;
    std::istringstream issL0ObjectsRange(kernelInfo);
    std::vector<std::string> objectsTable;
    std::string strObj;
    while (std::getline(issL0ObjectsRange, strObj, '/')) {
      objectsTable.push_back(strObj);
    }
    if (objectsTable.size() != 3) {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    auto pattern = std::regex("(\\d+)");
    auto iter = std::sregex_iterator(objectsTable[0].begin(), objectsTable[0].end(), pattern);
    auto end = std::sregex_iterator();
    auto size = std::distance(iter, end);
    auto& startCommandQueueSubmit = levelzero.recorder.kernel.startCommandQueueSubmit;
    auto& stopCommandQueueSubmit = levelzero.recorder.kernel.stopCommandQueueSubmit;
    startCommandQueueSubmit = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopCommandQueueSubmit = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopCommandQueueSubmit = startCommandQueueSubmit;
    } else {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range command queue submission";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startCommandQueueSubmit > stopCommandQueueSubmit) {
      Log(ERR) << "Incorrect config: LevelZero.Capture.Kernel.Range start command queue submission "
                  "can't be greater than stop command queue submission";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    iter = std::sregex_iterator(objectsTable[1].begin(), objectsTable[1].end(), pattern);
    size = std::distance(iter, end);
    auto& startCommandList = levelzero.recorder.kernel.startCommandList;
    auto& stopCommandList = levelzero.recorder.kernel.stopCommandList;
    startCommandList = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopCommandList = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopCommandList = startCommandList;
    } else {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range command list";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    iter = std::sregex_iterator(objectsTable[2].begin(), objectsTable[2].end(), pattern);
    size = std::distance(iter, end);
    auto& startKernel = levelzero.recorder.kernel.startKernel;
    auto& stopKernel = levelzero.recorder.kernel.stopKernel;
    startKernel = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopKernel = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopKernel = startKernel;
    } else {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range kernel";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startKernel > stopKernel) {
      Log(ERR) << "In config LevelZero.Capture.Kernel.Range start kernel "
                  "greater then stop kernel is not supported";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    levelzero.recorder.kernel.singleCapture = (startCommandQueueSubmit == stopCommandQueueSubmit) &&
                                              (startCommandList == stopCommandList) &&
                                              (startKernel == stopKernel);
  }
}

bool gits::Config::Set(const std::filesystem::path& cfgPath, const Config::TMode& mode) {
  static bool configured = false;
  if (configured) {
    return true;
  }

  auto configYaml = Config::LoadConfigFile(cfgPath);
  auto cfg = Config::Get();
  cfg.common.mode = mode;
  cfg.SetCommon(configYaml["Common"]);
  cfg.SetOpenGL(configYaml["OpenGL"]);
  cfg.SetVulkan(configYaml["Vulkan"]);
  cfg.SetOpenCL(configYaml["OpenCL"]);
#ifdef WITH_LEVELZERO
  cfg.SetLevelZero(configYaml["LevelZero"]);
#endif
  Config::Set(cfg);

  if (Config::Get().common.mode == MODE_RECORDER) {
    // create file data and register it in GITS
    CGits& inst = CGits::Instance();
    std::unique_ptr<CFile> file(new CFile(inst.Version()));
    auto& properties = file->GetProperties();
    gather_diagnostic_info(properties);
    properties["diag"]["gits"]["config_path"] = cfgPath;
    std::stringstream configStrStream;
    configStrStream << configYaml;
    properties["diag"]["gits"]["config"] = configStrStream.str();
    inst.RegisterFileRecorder(std::move(file));
  }

  configured = true;
  return true;
}

std::string gits::Config::Common::Recorder::PrepareDumpPath(const std::string& str,
                                                            bool uniqueDump) {
  if (str.empty()) {
    return "";
  }
  std::string preparedPath = str;
  if (uniqueDump) {
    auto time = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(time);
    char text[256];
    strftime(text, sizeof(text), "%y_%m_%d_%H_%M_%S", localtime(&t));
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) -
        std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
    preparedPath += "_" + std::string(text) + "." + std::to_string(ms.count());
  }

  //handle special names in dump dir path:
  //  move to somewhere in common if this grows
  //  %p% - process pid
  //  %n% - process name
  const std::string pidPlaceholder = "%p%";
  std::string::size_type pidPos = preparedPath.find(pidPlaceholder);
  const std::string namePlaceholder = "%n%";
  std::string::size_type namePos = preparedPath.find(namePlaceholder);

  if (pidPos != std::string::npos) {
    std::stringstream str;
    str << getpid();
    std::string left = preparedPath.substr(0, pidPos);
    std::string right = preparedPath.substr(pidPos + pidPlaceholder.size());
    preparedPath = left + str.str() + right;
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
    std::string left = preparedPath.substr(0, namePos);
    std::string right = preparedPath.substr(namePos + namePlaceholder.size());
    preparedPath = left + processname + right;
  }
  return preparedPath;
}

std::vector<std::string> gits::Config::LevelZero::ParseCaptureKernels(const std::string& value) {
  std::vector<std::string> objectsTable;
  std::istringstream issL0ObjectsRange(value);
  std::string strObj;
  char delim = '/';
  if (value.find_first_of('/') == std::string::npos) {
    delim = '\\';
  }
  while (std::getline(issL0ObjectsRange, strObj, delim)) {
    objectsTable.push_back(strObj);
  }
  return objectsTable;
}

bool gits::Config::IsRecorder() {
  return Get().common.mode == TMode::MODE_RECORDER;
}

bool gits::Config::IsPlayer() {
  return Get().common.mode == TMode::MODE_PLAYER;
}

bool gits::Config::dumpBinary() {
  return Get().common.recorder.recordingMode == TRecordingMode::BINARY;
}

bool gits::Config::dumpCCode() {
  return Get().common.recorder.recordingMode == TRecordingMode::CCODE;
}

bool gits::isTraceDataOptPresent(gits::TraceData option) {
  return gits::Config::Get().common.shared.traceDataOpts.find(option) !=
         gits::Config::Get().common.shared.traceDataOpts.end();
}

void gits::Config::Vulkan::Recorder::SetRangeSpecial(const std::string& val,
                                                     const gits::VulkanObjectMode& mode,
                                                     size_t expectedVecSize) {
  objRange.rangeSpecial.SetFromString(val);
  if (objRange.rangeSpecial.objVector.size() == expectedVecSize) {
    objRange.rangeSpecial.objMode = mode;
  } else {
    Log(ERR) << "Incorrect range for mode: " << mode;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}

#else
// Ccode only implementation.
bool gits::Config::IsRecorder() {
  return false;
}

bool gits::Config::IsPlayer() {
  return true;
}

bool gits::Config::dumpBinary() {
  return false;
}

bool gits::Config::dumpCCode() {
  return false;
}

bool gits::isTraceDataOptPresent(gits::TraceData option) {
  return false;
}

#endif

void gits::Config::Set(const Config& cfg) {
  Config& config_ = const_cast<Config&>(Get());
  config_ = cfg;
}

void gits::Config::SetMode(const Config::TMode& mode) {
  auto cfg = Config::Get();
  cfg.common.mode = mode;
  Config::Set(cfg);
}
