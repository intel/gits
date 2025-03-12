// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "yaml-cpp/yaml.h"
#include "configTypes.h"
#include "config.h"

#include <filesystem>

namespace YAML {

bool scalarAndNotNull(const Node& node) {
  if (node.IsNull()) {
    Log(ERR) << "Missing value";
    return false;
  }
  if (!node.IsScalar()) {
    return false;
  }
  return true;
}

template <>
struct convert<std::filesystem::path> {
  static bool decode(const Node& node, std::filesystem::path& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    rhs = std::filesystem::path(node.as<std::string>());
    return true;
  }
};

template <>
struct convert<gits::vi_bool> {
  static bool decode(const Node& node, gits::vi_bool& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    rhs = node.as<bool>();
    return true;
  }
};

template <>
struct convert<gits::vi_int> {
  static bool decode(const Node& node, gits::vi_int& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    std::string str = node.as<std::string>();
    std::stringstream sstream(str);
    if (str.compare(0, 2, "0x") == 0) {
      sstream << std::hex;
    }
    sstream >> rhs;
    return true;
  }
};

template <>
struct convert<gits::vi_uint> {
  static bool decode(const Node& node, gits::vi_uint& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    std::string str = node.as<std::string>();
    std::stringstream sstream(str);
    if (str.compare(0, 2, "0x") == 0) {
      sstream << std::hex;
    }
    sstream >> rhs;
    return true;
  }
};

template <>
struct convert<gits::vi_uint64> {
  static bool decode(const Node& node, gits::vi_uint64& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    std::string str = node.as<std::string>();
    std::stringstream sstream(str);
    if (str.compare(0, 2, "0x") == 0) {
      sstream << std::hex;
    }
    sstream >> rhs;
    return true;
  }
};

template <>
struct convert<BitRange> {
  static bool decode(const Node& node, BitRange& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    try {
      auto bitRangeVal = node.as<std::string>();
      if (bitRangeVal == "all") {
        rhs = BitRange(true);
      } else {
        rhs = BitRange(bitRangeVal);
      }
      return true;
    } catch (const std::exception& e) {
      Log(ERR) << "BitRange decoding error: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::LogLevel> {
  static bool decode(const Node& node, gits::LogLevel& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    auto val = node.as<std::string>();
    if (val.empty()) {
      Log(ERR) << "LogLevel has to be specified";
      return false;
    }

    gits::CEnumParser<gits::LogLevel> logLevelParser;
    std::optional<gits::LogLevel> lvl = logLevelParser.ParseEnum(val);
    if (lvl) {
      rhs = lvl.value();
      return true;
    }
    Log(ERR) << "LogLevel decoding error, cannot parse given value";
    return false;
  }
};

template <>
struct convert<gits::VulkanObjectRange> {
  static bool decode(const Node& node, gits::VulkanObjectRange& rhs) {
    if (!scalarAndNotNull(node)) {
      return false;
    }
    try {
      rhs.SetFromString(node.as<std::string>());
      return true;
    } catch (const std::exception& e) {
      Log(ERR) << "VulkanObjectRange decoding error: " << e.what();
      return false;
    }
  }
};

template <typename T>
bool decodeNamedValuesBaseOption(const Node& node, T& rhs) {
  if (!scalarAndNotNull(node)) {
    return false;
  }
  try {
    rhs.setFromString(node.as<std::string>());
    return true;
  } catch (const std::exception& e) {
    Log(ERR) << "NamedValuesBase decoding error: " << e.what();
    return false;
  }
}

template <>
struct convert<gits::RecordingModeOpt> {
  static bool decode(const Node& node, gits::RecordingModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::OpenGLRecorderModeOpt> {
  static bool decode(const Node& node, gits::OpenGLRecorderModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::VulkanRecorderModeOpt> {
  static bool decode(const Node& node, gits::VulkanRecorderModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::OpenCLRecorderModeOpt> {
  static bool decode(const Node& node, gits::OpenCLRecorderModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::LevelZeroRecorderModeOpt> {
  static bool decode(const Node& node, gits::LevelZeroRecorderModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::VKCaptureGroupTypeOpt> {
  static bool decode(const Node& node, gits::VKCaptureGroupTypeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::DeviceTypeOpt> {
  static bool decode(const Node& node, gits::DeviceTypeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::MemoryUpdateStateOpt> {
  static bool decode(const Node& node, gits::MemoryUpdateStateOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::MemoryTrackingModeOpt> {
  static bool decode(const Node& node, gits::MemoryTrackingModeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::BufferStateRestorationOpt> {
  static bool decode(const Node& node, gits::BufferStateRestorationOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::MemoryStateRestorationOpt> {
  static bool decode(const Node& node, gits::MemoryStateRestorationOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::GLProfileOpt> {
  static bool decode(const Node& node, gits::GLProfileOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::GLNativeApiOpt> {
  static bool decode(const Node& node, gits::GLNativeApiOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::BuffersStateOpt> {
  static bool decode(const Node& node, gits::BuffersStateOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::TexturesStateOpt> {
  static bool decode(const Node& node, gits::TexturesStateOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::WindowsKeyHandlingOpt> {
  static bool decode(const Node& node, gits::WindowsKeyHandlingOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::VkRenderDocCaptureOpt> {
  static bool decode(const Node& node, gits::VkRenderDocCaptureOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::CompressionTypeOpt> {
  static bool decode(const Node& node, gits::CompressionTypeOpt& rhs) {
    return decodeNamedValuesBaseOption(node, rhs);
  }
};

template <>
struct convert<gits::Config::Common::Shared> {
  static bool decode(const Node& node, gits::Config::Common::Shared& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.thresholdLogLevel = node["LogLevel"].as<gits::LogLevel>();
      rhs.logToConsole = node["LogToConsole"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::Common::Player> {
  static bool decode(const Node& node, gits::Config::Common::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.libGL = node["LibGL"].as<std::filesystem::path>();
      rhs.libEGL = node["LibEGL"].as<std::filesystem::path>();
      rhs.libGLESv1 = node["LibGLES1"].as<std::filesystem::path>();
      rhs.libGLESv2 = node["LibGLES2"].as<std::filesystem::path>();
      rhs.libClPath = node["LibCL"].as<std::filesystem::path>();
      rhs.libVK = node["LibVK"].as<std::filesystem::path>();
      rhs.libOcloc = node["LibOcloc"].as<std::filesystem::path>();
      rhs.libL0 = node["LibL0"].as<std::filesystem::path>();

      rhs.streamPath = node["StreamPath"].as<std::filesystem::path>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.subcapturePath = node["SubcapturePath"].as<std::filesystem::path>();
#endif
      rhs.tokenBurst = node["TokenBurstLimit"].as<gits::vi_uint>();
      rhs.tokenBurstNum = node["TokenBurstNum"].as<gits::vi_uint>();
      rhs.exitFrame = node["ExitFrame"].as<gits::vi_uint>();
      rhs.endFrameSleep = node["EndFrameSleep"].as<gits::vi_uint>();
      rhs.exitOnError = node["ExitOnError"].as<gits::vi_bool>();
      rhs.eventScript = node["EventScript"].as<std::filesystem::path>();
      rhs.scriptArgsStr = node["ScriptArgs"].as<std::string>();
      rhs.benchmark = node["Benchmark"].as<gits::vi_bool>();
      rhs.showWindowBorder = node["ShowWindowBorder"].as<gits::vi_bool>();
      rhs.captureFrames = node["CaptureFrames"].as<BitRange>();
      rhs.outputDir = node["OutputDir"].as<std::filesystem::path>();
      rhs.traceSelectedFrames = node["TraceSelectedFrames"].as<BitRange>();
      rhs.interactive = node["Interactive"].as<gits::vi_bool>();
      rhs.outputTracePath = node["OutputTracePath"].as<std::filesystem::path>();
      rhs.logFncs = node["LogFncs"].as<gits::vi_bool>();
      rhs.faithfulThreading = node["FaithfulThreading"].as<gits::vi_bool>();
      rhs.loadWholeStreamBeforePlayback = node["LoadWholeStreamBeforePlayback"].as<gits::vi_bool>();
      rhs.showWindowsWA = node["ShowWindowsWA"].as<gits::vi_bool>();
      rhs.disableExceptionHandling = node["DisableExceptionHandling"].as<gits::vi_bool>();
      rhs.captureScreenshot = node["CaptureScreenshot"].as<gits::vi_bool>();
      rhs.logLoadedTokens = node["LogLoadedTokens"].as<gits::vi_bool>();
      rhs.escalatePriority = node["EscalatePriority"].as<gits::vi_bool>();
      rhs.swapAfterPrepare = node["SwapAfterPrepare"].as<gits::vi_bool>();
      rhs.stopAfterFrames = node["StopAfterFrames"].as<BitRange>();
      rhs.nullRun = node["NullRun"].as<gits::vi_bool>();
      rhs.waitForEnter = node["WaitForEnter"].as<gits::vi_bool>();
      rhs.cleanResourcesOnExit = node["CleanResourcesOnExit"].as<gits::vi_bool>();
      rhs.renderOffscreen = node["RenderOffscreen"].as<gits::vi_bool>();
      rhs.forceOrigScreenResolution = node["ForceOrigScreenResolution"].as<gits::vi_bool>();
      rhs.forceInvisibleWindows = node["ForceInvisibleWindows"].as<gits::vi_bool>();
      rhs.fullscreen = node["Fullscreen"].as<gits::vi_bool>();
      rhs.forceWindowPos.enabled = node["ForceWindowPos"]["Enabled"].as<gits::vi_bool>();
      rhs.forceWindowPos.x = node["ForceWindowPos"]["x"].as<gits::vi_uint>();
      rhs.forceWindowPos.y = node["ForceWindowPos"]["y"].as<gits::vi_uint>();
      rhs.forceWindowSize.enabled = node["ForceWindowSize"]["Enabled"].as<gits::vi_bool>();
      rhs.forceWindowSize.width = node["ForceWindowSize"]["Width"].as<gits::vi_uint>();
      rhs.forceWindowSize.height = node["ForceWindowSize"]["Height"].as<gits::vi_uint>();
      rhs.forceScissor.enabled = node["ForceScissor"]["Enabled"].as<gits::vi_bool>();
      rhs.forceScissor.x = node["ForceScissor"]["x"].as<gits::vi_uint>();
      rhs.forceScissor.y = node["ForceScissor"]["y"].as<gits::vi_uint>();
      rhs.forceScissor.width = node["ForceScissor"]["Width"].as<gits::vi_uint>();
      rhs.forceScissor.height = node["ForceScissor"]["Height"].as<gits::vi_uint>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.forceDesktopResolution.enabled =
          node["ForceDesktopResolution"]["Enabled"].as<gits::vi_bool>();
      rhs.forceDesktopResolution.width =
          node["ForceDesktopResolution"]["Width"].as<gits::vi_uint>();
      rhs.forceDesktopResolution.height =
          node["ForceDesktopResolution"]["Height"].as<gits::vi_uint>();
#endif
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::Common::Recorder> {
  static bool decode(const Node& node, gits::Config::Common::Recorder& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.recordingMode = node["RecordingMode"].as<gits::RecordingModeOpt>();
      rhs.exitKeysStr = node["ExitKeys"].as<std::vector<std::string>>();
      rhs.exitAfterAPICall = node["ExitAfterAPICall"].as<gits::vi_uint>();
#ifndef GITS_PLATFORM_WINDOWS
      rhs.exitSignal = node["ExitSignal"].as<gits::vi_uint>();
#endif
      rhs.tokenBurst = node["TokenBurstLimit"].as<gits::vi_uint>();
      rhs.tokenBurstNum = node["TokenBurstNum"].as<gits::vi_uint>();

      rhs.libGL = node["LibGL"].as<std::filesystem::path>();
      rhs.libEGL = node["LibEGL"].as<std::filesystem::path>();
      rhs.libGLESv1 = node["LibGLES1"].as<std::filesystem::path>();
      rhs.libGLESv2 = node["LibGLES2"].as<std::filesystem::path>();
      rhs.libClPath = node["LibCL"].as<std::filesystem::path>();
      rhs.libVK = node["LibVK"].as<std::filesystem::path>();
      rhs.libOcloc = node["LibOcloc"].as<std::filesystem::path>();
      rhs.libL0 = node["LibL0"].as<std::filesystem::path>();

      rhs.installPath = node["InstallationPath"].as<std::filesystem::path>();
      rhs.dumpPath = node["DumpDirectoryPath"].as<std::filesystem::path>();
      rhs.uniqueDumpDirectory = node["UniqueDumpDirectory"].as<gits::vi_bool>();
      rhs.eventScript = node["EventScript"].as<std::filesystem::path>();
      rhs.scriptArgsStr = node["ScriptArgs"].as<std::string>();
      rhs.compression.type = node["Compression"]["Type"].as<gits::CompressionTypeOpt>();
      rhs.compression.level = node["Compression"]["Level"].as<gits::vi_uint>();
      rhs.compression.chunkSize = node["Compression"]["ChunkSize"].as<gits::vi_uint>();
      rhs.extendedDiagnosticInfo = node["ExtendedDiagnostic"].as<gits::vi_bool>();
      rhs.forceDumpOnError = node["ForceDumpOnError"].as<gits::vi_bool>();
      rhs.zipTextFiles = node["ZipTextFiles"].as<gits::vi_bool>();
      rhs.highIntegrity = node["HighIntegrity"].as<gits::vi_bool>();
      rhs.nullIO = node["NullIO"].as<gits::vi_bool>();
      rhs.removeDXSharing = node["RemoveDXSharing"].as<gits::vi_bool>();
      rhs.removeGLSharing = node["RemoveGLSharing"].as<gits::vi_bool>();
      rhs.benchmark = node["Benchmark"].as<gits::vi_bool>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.closeAppOnStopRecording = node["CloseAppOnStopRecording"].as<gits::vi_bool>();
      rhs.windowsKeyHandling = node["WindowsKeyHandling"].as<gits::WindowsKeyHandlingOpt>();
#endif
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::OpenGL::Shared> {
  static bool decode(const Node& node, gits::Config::OpenGL::Shared& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.traceGLError = node["TraceGLError"].as<gits::vi_bool>();
      rhs.forceGLVersion = node["ForceGLVersion"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::OpenGL::Player> {
  static bool decode(const Node& node, gits::Config::OpenGL::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.forceGLProfile = node["ForceGLProfile"].as<gits::GLProfileOpt>();
      rhs.forceGLNativeAPI = node["ForceGLNativeAPI"].as<gits::GLNativeApiOpt>();
      rhs.skipQueries = node["SkipQueries"].as<gits::vi_bool>();
      rhs.scaleFactor = node["ScaleFactor"].as<float>();
      rhs.captureFramesHashes = node["CaptureFramesHashes"].as<gits::vi_bool>();
      rhs.dontForceBackBufferGL = node["DontForceBackBufferGL"].as<gits::vi_bool>();
      rhs.captureWholeWindow = node["CaptureWholeWindow"].as<gits::vi_bool>();
      rhs.capture2DTexs = node["Capture2DTexs"].as<BitRange>();
      rhs.captureDraws2DTexs = node["CaptureDraws2DTexs"].as<BitRange>();
      rhs.captureDraws = node["CaptureDraws"].as<BitRange>();
      rhs.captureDrawsPre = node["CaptureDrawsPre"].as<gits::vi_bool>();
      rhs.captureFinishFrame = node["CaptureFinishFrame"].as<BitRange>();
      rhs.captureReadPixels = node["CaptureReadPixels"].as<BitRange>();
      rhs.captureFlushFrame = node["CaptureFlushFrame"].as<BitRange>();
      rhs.captureBindFboFrame = node["CaptureBindFboFrame"].as<BitRange>();
      rhs.keepDraws = node["KeepDraws"].as<BitRange>();
      rhs.keepFrames = node["KeepFrames"].as<BitRange>();
      rhs.minimalConfig = node["MinimalConfig"].as<gits::vi_bool>();
      rhs.traceGitsInternal = node["TraceGitsInternal"].as<gits::vi_bool>();
      rhs.linkGetProgBinary = node["LinkGetProgBinary"].as<gits::vi_bool>();
      rhs.linkUseProgBinary = node["LinkUseProgBinary"].as<gits::vi_bool>();
      rhs.affectedViewport = node["AffectedViewport"].as<std::vector<int>>();
      rhs.traceGLBufferHashes = node["TraceGLBufferHashes"].as<BitRange>();
      rhs.forceNoMSAA = node["ForceNoMSAA"].as<gits::vi_bool>();
      rhs.destroyContextsOnExit = node["DestroyContextsOnExit"].as<gits::vi_bool>();
#ifdef GITS_PLATFORM_LINUX
      rhs.forceWaylandWindow = node["ForceWaylandWindow"].as<gits::vi_bool>();
#endif
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::OpenGL::Recorder> {
  static bool decode(const Node& node, gits::Config::OpenGL::Recorder& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.mode = node["Mode"].as<gits::OpenGLRecorderModeOpt>();
      rhs.all.exitFrame = node["All"]["ExitFrame"].as<gits::vi_uint>();
      rhs.all.exitDeleteContext = node["All"]["ExitDeleteContext"].as<gits::vi_uint>();
      rhs.frames.startFrame = node["Frames"]["StartFrame"].as<gits::vi_uint>();
      rhs.frames.stopFrame = node["Frames"]["StopFrame"].as<gits::vi_uint>();
      rhs.frames.startKeysStr = node["Frames"]["StartKeys"].as<std::vector<std::string>>();
      rhs.frames.frameSeparators.glFinishSep =
          node["Frames"]["FrameSeparators"]["glFinish"].as<gits::vi_bool>();
      rhs.frames.frameSeparators.glFlushSep =
          node["Frames"]["FrameSeparators"]["glFlush"].as<gits::vi_bool>();
      rhs.oglSingleDraw.number = node["OglSingleDraw"]["Number"].as<gits::vi_uint>();
      rhs.oglDrawsRange.startDraw = node["OglDrawsRange"]["StartDraw"].as<gits::vi_uint>();
      rhs.oglDrawsRange.stopDraw = node["OglDrawsRange"]["StopDraw"].as<gits::vi_uint>();
      rhs.oglDrawsRange.frame = node["OglDrawsRange"]["Frame"].as<gits::vi_uint>();
      rhs.dumpScreenshots = node["DumpScreenshots"].as<BitRange>();
      rhs.dumpDrawsFromFrames = node["DumpDrawsFromFrames"].as<BitRange>();
      rhs.suppressExtensions = node["SuppressExtensions"].as<std::vector<std::string>>();
      rhs.suppressProgramBinary = node["SuppressProgramBinary"].as<gits::vi_bool>();
      rhs.endFrameSleep = node["EndFrameSleep"].as<gits::vi_uint>();
      rhs.restoreDefaultFB = node["RestoreDefaultFB"].as<gits::vi_bool>();
      rhs.doNotRemoveWin = node["DoNotRemoveWindow"].as<gits::vi_bool>();
      rhs.multiApiProtectBypass = node["MultiApiProtectBypass"].as<gits::vi_bool>();
      rhs.carrayMemCmpType = node["CArrayMemCmpType"].as<gits::vi_uint>();
      rhs.stripIndicesValues = node["StripIndicesValues"].as<gits::vi_uint>();
      rhs.optimizeBufferSize = node["OptimizeBufferSize"].as<gits::vi_bool>();
      rhs.retryFunctionLoads = node["RetryFunctionLoads"].as<gits::vi_bool>();
      rhs.detectRecursion = node["DetectRecursion"].as<gits::vi_bool>();
      rhs.buffersState = node["BuffersState"].as<gits::BuffersStateOpt>();
      rhs.texturesState = node["TexturesState"].as<gits::TexturesStateOpt>();
      rhs.coherentMapUpdatePerFrame = node["CoherentMapUpdatePerFrame"].as<gits::vi_bool>();
      rhs.bufferMapAccessMask = node["BufferMapAccessMask"].as<gits::vi_uint>();
      rhs.bufferStorageFlagsMask = node["BufferStorageFlagsMask"].as<gits::vi_uint>();
      rhs.coherentMapBehaviorWA = node["CoherentMapBehaviorWA"].as<gits::vi_bool>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.schedulefboEXTAsCoreWA = node["ScheduleFboEXTAsCoreWA"].as<gits::vi_bool>();
      rhs.useGlGetTexImageAndRestoreBuffersWhenPossibleES =
          node["UseGlGetTexImageAndRestoreBuffersWhenPossibleES"].as<gits::vi_bool>();
      rhs.trackTextureBindingWA = node["TrackTextureBindingWA"].as<gits::vi_bool>();
      rhs.forceBuffersStateCaptureAlwaysWA =
          node["ForceBuffersStateCaptureAlwaysWA"].as<gits::vi_bool>();
      rhs.restoreIndexedTexturesWA = node["RestoreIndexedTexturesWA"].as<gits::vi_bool>();
      rhs.mtDriverWA = node["MTDriverWA"].as<gits::vi_bool>();
#endif
      rhs.ccodeRangesWA = node["CCodeRangesWA"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::Vulkan::Shared> {
  static bool decode(const Node& node, gits::Config::Vulkan::Shared& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.suppressPhysicalDeviceFeatures =
          node["SuppressPhysicalDeviceFeatures"].as<std::vector<std::string>>();
      rhs.suppressExtensions = node["SuppressExtensions"].as<std::vector<std::string>>();
      rhs.suppressLayers = node["SuppressLayers"].as<std::vector<std::string>>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::Vulkan::Player> {
  static bool decode(const Node& node, gits::Config::Vulkan::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.exitOnVkQueueSubmitFail = node["ExitOnVkQueueSubmitFail"].as<gits::vi_bool>();
      rhs.captureVulkanSubmits = node["CaptureVulkanSubmits"].as<BitRange>();
      rhs.captureVulkanSubmitsResources = node["CaptureVulkanSubmitsResources"].as<BitRange>();
      rhs.captureVulkanSubmitsGroupType =
          node["CaptureVulkanSubmitsGroupType"].as<gits::VKCaptureGroupTypeOpt>();
      rhs.captureVulkanRenderPasses =
          node["CaptureVulkanRenderPasses"].as<gits::VulkanObjectRange>();
      rhs.captureVulkanRenderPassesResources =
          node["CaptureVulkanRenderPassesResources"].as<gits::VulkanObjectRange>();
      rhs.captureVulkanDraws = node["CaptureVulkanDraws"].as<gits::VulkanObjectRange>();
      rhs.captureVulkanResources = node["CaptureVulkanResources"].as<gits::VulkanObjectRange>();
      rhs.skipNonDeterministicImages = node["SkipNonDeterministicImages"].as<gits::vi_bool>();
      rhs.ignoreVKCrossPlatformIncompatibilitiesWA =
          node["IgnoreVKCrossPlatformIncompatibilitiesWA"].as<gits::vi_bool>();
      rhs.waitAfterQueueSubmitWA = node["WaitAfterQueueSubmitWA"].as<gits::vi_bool>();
      rhs.traceVKShaderHashes = node["TraceVKShaderHashes"].as<gits::vi_bool>();
      rhs.maxAllowedVkSwapchainRewinds = node["MaxAllowedVkSwapchainRewinds"].as<gits::vi_uint>();
      rhs.overrideVKPipelineCache = node["OverrideVKPipelineCache"].as<std::filesystem::path>();
      rhs.oneVulkanDrawPerCommandBuffer = node["OneVulkanDrawPerCommandBuffer"].as<gits::vi_bool>();
      rhs.oneVulkanRenderPassPerCommandBuffer =
          node["OneVulkanRenderPassPerCommandBuffer"].as<gits::vi_bool>();
      rhs.vulkanForcedPhysicalDeviceIndex = node["ForcedPhysicalDeviceIndex"].as<gits::vi_uint>();
      rhs.vulkanForcedPhysicalDeviceName = node["ForcedPhysicalDeviceName"].as<std::string>();
      rhs.vulkanForcedPhysicalDeviceType =
          node["ForcedPhysicalDeviceType"].as<gits::DeviceTypeOpt>();
      rhs.printStateRestoreLogsVk = node["PrintStateRestoreLogsVk"].as<gits::vi_bool>();
      rhs.printMemUsageVk = node["PrintMemUsageVk"].as<gits::vi_bool>();
      rhs.forceMultithreadedPipelineCompilation =
          node["ForceMultithreadedPipelineCompilation"].as<gits::vi_bool>();
      rhs.execCmdBuffsBeforeQueueSubmit = node["ExecCmdBuffsBeforeQueueSubmit"].as<gits::vi_bool>();
      rhs.patchShaderGroupHandlesInSBT = node["PatchShaderGroupHandlesInSBT"].as<gits::vi_bool>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.renderDoc.mode = node["RenderDoc"]["Mode"].as<gits::VkRenderDocCaptureOpt>();
      rhs.renderDoc.captureRange = node["RenderDoc"]["Range"].as<BitRange>();
      rhs.renderDoc.continuousCapture = node["RenderDoc"]["ContinuousCapture"].as<gits::vi_bool>();
      rhs.renderDoc.enableUI = node["RenderDoc"]["EnableUI"].as<gits::vi_bool>();
#endif
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::Vulkan::Recorder> {
  static bool decode(const Node& node, gits::Config::Vulkan::Recorder& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.mode = node["Mode"].as<gits::VulkanRecorderModeOpt>();
      rhs.all.exitFrame = node["All"]["ExitFrame"].as<gits::vi_uint>();
      rhs.frames.startFrame = node["Frames"]["StartFrame"].as<gits::vi_uint>();
      rhs.frames.stopFrame = node["Frames"]["StopFrame"].as<gits::vi_uint>();
      rhs.frames.startKeysStr = node["Frames"]["StartKeys"].as<std::vector<std::string>>();

      rhs.queueSubmitStr = node["QueueSubmit"]["Number"].as<std::string>();
      rhs.commandBuffersRangeStr = node["CommandBuffersRange"]["Range"].as<std::string>();
      rhs.renderPassRangeStr = node["RenderPassRange"]["Range"].as<std::string>();
      rhs.drawsRangeStr = node["DrawsRange"]["Range"].as<std::string>();
      rhs.dispatchRangeStr = node["DispatchRange"]["Range"].as<std::string>();
      rhs.blitRangeStr = node["BlitRange"]["Range"].as<std::string>();

      rhs.dumpScreenshots = node["DumpScreenshots"].as<BitRange>();
      rhs.dumpSubmits = node["DumpSubmits"].as<BitRange>();
      rhs.traceVkStructs = node["TraceVKStructs"].as<gits::vi_bool>();
      rhs.memorySegmentSize = node["MemorySegmentSize"].as<gits::vi_uint>();
      rhs.memoryTrackingMode = node["MemoryTrackingMode"].as<gits::MemoryTrackingModeOpt>();
      rhs.memoryUpdateState = node["MemoryUpdateState"].as<gits::MemoryUpdateStateOpt>();
      rhs.forceUniversalRecording = node["ForceUniversalRecording"].as<gits::vi_bool>();
      rhs.delayFenceChecksCount = node["DelayFenceChecksCount"].as<gits::vi_uint>();
      rhs.shortenFenceWaitTime = node["ShortenFenceWaitTime"].as<gits::vi_uint>();
      rhs.addImageUsageFlags = node["AddImageUsageFlags"].as<gits::vi_uint>();
      rhs.addBufferUsageFlags = node["AddBufferUsageFlags"].as<gits::vi_uint>();
      rhs.scheduleCommandBuffersBeforeQueueSubmit =
          node["ScheduleCommandBuffersBeforeQueueSubmitWA"].as<gits::vi_bool>();
      rhs.minimalStateRestore = node["MinimalStateRestore"].as<gits::vi_bool>();
      rhs.reusableStateRestoreResourcesCount =
          node["ReusableStateRestoreResourcesCount"].as<gits::vi_uint>();
      rhs.reusableStateRestoreBufferSize =
          node["ReusableStateRestoreBufferSize"].as<gits::vi_uint>();
      rhs.increaseImageMemorySizeRequirement.fixedAmount =
          node["IncreaseImageMemorySizeRequirement"]["FixedAmount"].as<gits::vi_uint>();
      rhs.increaseImageMemorySizeRequirement.percent =
          node["IncreaseImageMemorySizeRequirement"]["Percent"].as<gits::vi_uint>();
      rhs.memoryOffsetAlignmentOverride.images =
          node["MemoryOffsetAlignmentOverride"]["Images"].as<gits::vi_uint>();
      rhs.memoryOffsetAlignmentOverride.buffers =
          node["MemoryOffsetAlignmentOverride"]["Buffers"].as<gits::vi_uint>();
      rhs.memoryOffsetAlignmentOverride.descriptors =
          node["MemoryOffsetAlignmentOverride"]["Descriptors"].as<gits::vi_uint>();
      rhs.crossPlatformStateRestoration.images =
          node["CrossPlatformStateRestoration"]["Images"].as<gits::vi_bool>();
      rhs.crossPlatformStateRestoration.buffers =
          node["CrossPlatformStateRestoration"]["Buffers"].as<gits::BufferStateRestorationOpt>();
      rhs.memoryRestoration = node["MemoryRestoration"].as<gits::MemoryStateRestorationOpt>();
      rhs.restoreMultisampleImagesWA = node["RestoreMultisampleImagesWA"].as<gits::vi_bool>();
      rhs.maxArraySizeForCCode = node["MaxArraySizeForCCode"].as<gits::vi_uint>();
      rhs.useCaptureReplayFeaturesForBuffersAndAccelerationStructures =
          node["UseCaptureReplayFeaturesForBuffersAndAccelerationStructures"].as<gits::vi_bool>();
      rhs.useCaptureReplayFeaturesForRayTracingPipelines =
          node["UseCaptureReplayFeaturesForRayTracingPipelines"].as<gits::vi_bool>();
#ifdef GITS_PLATFORM_WINDOWS
      rhs.renderDocCompatibility =
          node["UsePresentSrcLayoutTransitionAsAFrameBoundary"].as<gits::vi_bool>();
      rhs.renderDocCompatibility = node["RenderDocCompatibility"].as<gits::vi_bool>();
#endif
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::OpenCL::Player> {
  static bool decode(const Node& node, gits::Config::OpenCL::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.captureImages = node["CaptureImages"].as<gits::vi_bool>();
      rhs.removeSourceLengths = node["RemoveSourceLengths"].as<gits::vi_bool>();
      rhs.captureReads = node["CaptureReads"].as<gits::vi_bool>();
      rhs.captureKernels = node["CaptureKernels"].as<BitRange>();
      rhs.omitReadOnlyObjects = node["OmitReadOnlyObjects"].as<gits::vi_bool>();
      rhs.dumpLayoutOnly = node["DumpLayoutOnly"].as<gits::vi_bool>();
      rhs.injectBufferResetAfterCreate = node["InjectBufferResetAfterCreate"].as<gits::vi_bool>();
      rhs.disableNullIndirectPointersInBuffer =
          node["DisableNullIndirectPointersInBuffer"].as<gits::vi_bool>();
      rhs.noOpenCL = node["NoOpenCL"].as<gits::vi_bool>();
      rhs.aubSignaturesCL = node["AubSignaturesCL"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::OpenCL::Recorder> {
  static bool decode(const Node& node, gits::Config::OpenCL::Recorder& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.mode = node["Mode"].as<gits::OpenCLRecorderModeOpt>();
      rhs.oclSingleKernel.number = node["OclSingleKernel"]["Number"].as<gits::vi_uint>();
      rhs.oclKernelsRange.startKernel = node["OclKernelsRange"]["StartKernel"].as<gits::vi_uint>();
      rhs.oclKernelsRange.stopKernel = node["OclKernelsRange"]["StopKernel"].as<gits::vi_uint>();
      rhs.dumpKernels = node["DumpKernels"].as<BitRange>();
      rhs.dumpImages = node["DumpImages"].as<gits::vi_bool>();
      rhs.omitReadOnlyObjects = node["OmitReadOnlyObjects"].as<gits::vi_bool>();
      rhs.bufferResetAfterCreate = node["BufferResetAfterCreate"].as<gits::vi_bool>();
      rhs.nullIndirectPointersInBuffer = node["NullIndirectPointersInBuffer"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::LevelZero::Player> {
  static bool decode(const Node& node, gits::Config::LevelZero::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.captureImages = node["CaptureImages"].as<gits::vi_bool>();
      rhs.dumpSpv = node["DumpSpv"].as<gits::vi_bool>();
      rhs.captureKernelsStr = node["CaptureKernels"].as<std::string>();
      rhs.dumpLayoutOnly = node["DumpLayoutOnly"].as<gits::vi_bool>();
      rhs.captureAfterSubmit = node["CaptureAfterSubmit"].as<gits::vi_bool>();
      rhs.captureInputKernels = node["CaptureInputKernels"].as<gits::vi_bool>();
      rhs.injectBufferResetAfterCreate = node["InjectBufferResetAfterCreate"].as<gits::vi_bool>();
      rhs.disableNullIndirectPointersInBuffer =
          node["DisableNullIndirectPointersInBuffer"].as<gits::vi_bool>();
      rhs.disableAddressTranslation = node["DisableAddressTranslation"].as<gits::vi_uint>();
      rhs.omitOriginalAddressCheck = node["OmitOriginalAddressCheck"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::LevelZero::Recorder> {
  static bool decode(const Node& node, gits::Config::LevelZero::Recorder& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.mode = node["Mode"].as<gits::LevelZeroRecorderModeOpt>();
      rhs.kernelRangeStr = node["Kernel"]["Range"].as<std::string>();
      rhs.dumpKernelsStr = node["DumpKernels"].as<std::string>();
      rhs.captureAfterSubmit = node["DumpAfterSubmit"].as<gits::vi_bool>();
      rhs.captureImages = node["DumpImages"].as<gits::vi_bool>();
      rhs.dumpInputKernels = node["DumpInputKernels"].as<gits::vi_bool>();
      rhs.bufferResetAfterCreate = node["BufferResetAfterCreate"].as<gits::vi_bool>();
      rhs.nullIndirectPointersInBuffer = node["NullIndirectPointersInBuffer"].as<gits::vi_bool>();
      rhs.bruteForceScanForIndirectPointers.memoryType =
          node["BruteForceScanForIndirectPointers"]["MemoryType"].as<gits::vi_uint>();
      rhs.bruteForceScanForIndirectPointers.iterations =
          node["BruteForceScanForIndirectPointers"]["Iterations"].as<gits::vi_uint>();
      rhs.disableAddressTranslation.memoryType =
          node["DisableAddressTranslation"]["MemoryType"].as<gits::vi_uint>();
      rhs.disableAddressTranslation.virtualDeviceMemorySize =
          node["DisableAddressTranslation"]["VirtualDeviceMemorySize"].as<gits::vi_uint64>();
      rhs.disableAddressTranslation.virtualHostMemorySize =
          node["DisableAddressTranslation"]["VirtualHostMemorySize"].as<gits::vi_uint64>();
      rhs.dumpLayoutOnly = node["DumpLayoutOnly"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

#ifdef GITS_PLATFORM_WINDOWS
template <>
struct convert<gits::Config::DirectX::Capture> {
  static bool decode(const Node& node, gits::Config::DirectX::Capture& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.record = node["Record"].as<gits::vi_bool>();
      rhs.shadowMemory = node["ShadowMemory"].as<gits::vi_bool>();
      rhs.captureIntelExtensions = node["CaptureIntelExtensions"].as<gits::vi_bool>();
      rhs.captureXess = node["CaptureXess"].as<gits::vi_bool>();
      rhs.captureDirectML = node["CaptureDirectML"].as<gits::vi_bool>();
      rhs.captureDirectStorage = node["CaptureDirectStorage"].as<gits::vi_bool>();
      rhs.debugLayer = node["DebugLayer"].as<gits::vi_bool>();
      rhs.plugins = node["Plugins"].as<std::vector<std::string>>();
      rhs.tokenBurstChunkSize = node["TokenBurstChunkSize"].as<gits::vi_uint64>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Player::AdapterOverride> {
  static bool decode(const Node& node, gits::Config::DirectX::Player::AdapterOverride& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.index = node["Index"].as<gits::vi_uint>();
      rhs.vendor = node["Vendor"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Player::ApplicationInfoOverride> {
  static bool decode(const Node& node,
                     gits::Config::DirectX::Player::ApplicationInfoOverride& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.applicationName = node["ApplicationName"].as<std::string>();
      rhs.applicationVersion = node["ApplicationVersion"].as<std::string>();
      rhs.engineName = node["EngineName"].as<std::string>();
      rhs.engineVersion = node["EngineVersion"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Player> {
  static bool decode(const Node& node, gits::Config::DirectX::Player& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.execute = node["Execute"].as<gits::vi_bool>();
      rhs.debugLayer = node["DebugLayer"].as<gits::vi_bool>();
      rhs.waitOnEventCompletion = node["WaitOnEventCompletion"].as<gits::vi_bool>();
      rhs.useCopyQueueOnRestore = node["UseCopyQueueOnRestore"].as<gits::vi_bool>();
      rhs.uavBarrierAfterCopyRaytracingASWorkaround =
          node["UavBarrierAfterCopyRaytracingASWorkaround"].as<gits::vi_bool>();
      rhs.skipResolveQueryData = node["SkipResolveQueryData"].as<gits::vi_bool>();
      rhs.multithreadedShaderCompilation =
          node["MultithreadedShaderCompilation"].as<gits::vi_bool>();
      rhs.plugins = node["Plugins"].as<std::vector<std::string>>();
      rhs.tokenBurstChunkSize = node["TokenBurstChunkSize"].as<gits::vi_uint64>();
      rhs.adapterOverride =
          node["AdapterOverride"].as<gits::Config::DirectX::Player::AdapterOverride>();
      rhs.applicationInfoOverride =
          node["ApplicationInfoOverride"]
              .as<gits::Config::DirectX::Player::ApplicationInfoOverride>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::Trace> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::Trace& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.flushMethod = node["FlushMethod"].as<std::string>();
      rhs.print.postCalls = node["Print"]["PostCalls"].as<gits::vi_bool>();
      rhs.print.preCalls = node["Print"]["PreCalls"].as<gits::vi_bool>();
      rhs.print.debugLayerWarnings = node["Print"]["DebugLayerWarnings"].as<gits::vi_bool>();
      rhs.print.gpuExecution = node["Print"]["GPUExecution"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::Subcapture> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::Subcapture& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.serializeAccelerationStructures =
          node["SerializeAccelerationStructures"].as<gits::vi_bool>();
      rhs.restoreTLASes = node["RestoreTLASes"].as<gits::vi_bool>();
      rhs.frames = node["Frames"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::Screenshots> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::Screenshots& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.frames = node["Frames"].as<std::string>();
      rhs.format = node["Format"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::ResourcesDump> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::ResourcesDump& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.resourceKeys = node["ResourceKeys"].as<std::string>();
      rhs.commandKeys = node["CommandKeys"].as<std::string>();
      rhs.textureRescaleRange = node["TextureRescaleRange"].as<std::string>();
      rhs.format = node["Format"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::RenderTargetsDump> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::RenderTargetsDump& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.frames = node["Frames"].as<std::string>();
      rhs.draws = node["Draws"].as<std::string>();
      rhs.format = node["Format"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::RaytracingDump> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::RaytracingDump& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.bindingTablesPre = node["BindingTablesPre"].as<gits::vi_bool>();
      rhs.bindingTablesPost = node["BindingTablesPost"].as<gits::vi_bool>();
      rhs.instancesPre = node["InstancesPre"].as<gits::vi_bool>();
      rhs.instancesPost = node["InstancesPost"].as<gits::vi_bool>();
      rhs.blases = node["BLASes"].as<gits::vi_bool>();
      rhs.commandKeys = node["CommandKeys"].as<std::string>();
      rhs.commandListModuloStep = node["CommandListModuloStep"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::ExecuteIndirectDump> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::ExecuteIndirectDump& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.argumentBufferPre = node["ArgumentBufferPre"].as<gits::vi_bool>();
      rhs.argumentBufferPost = node["ArgumentBufferPost"].as<gits::vi_bool>();
      rhs.commandKeys = node["CommandKeys"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::SkipCalls> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::SkipCalls& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.commandKeys = node["CommandKeys"].as<std::string>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

template <>
struct convert<gits::Config::DirectX::Features::Portability> {
  static bool decode(const Node& node, gits::Config::DirectX::Features::Portability& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    try {
      rhs.enabled = node["Enabled"].as<gits::vi_bool>();
      rhs.storePlacedResourceDataOnCapture =
          node["StorePlacedResourceDataOnCapture"].as<gits::vi_bool>();
      rhs.storePlacedResourceDataOnPlayback =
          node["StorePlacedResourceDataOnPlayback"].as<gits::vi_bool>();
      return true;
    } catch (const YAML::Exception& e) {
      Log(ERR) << "YAML parser exception: " << e.what();
      return false;
    }
  }
};

#endif

} // namespace YAML
