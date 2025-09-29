// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
// This file is auto-generated, manual changes will be lost on next run.
//

#include "deriveData.h"

#include <regex>

#include "configurationLib.h"

#include "lua_bindings.h"
#include "configUtils.h"
#include "gits.h"
#include "tools.h"
#include "exception.h"
#include "diagnostic.h"
#include "log2.h"

namespace {
inline std::vector<std::string> ParseCaptureKernels(const std::string& value) {
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
} // namespace

namespace gits {
template <>
void DeriveConfigData<Configuration>(Configuration& obj, Configuration& config) {
  obj.ccode.DeriveData(config);
  obj.common.DeriveData(config);
  obj.opengl.DeriveData(config);
  obj.vulkan.DeriveData(config);
  obj.directx.DeriveData(config);
  obj.opencl.DeriveData(config);
  obj.levelzero.DeriveData(config);
}

template <>
void DeriveConfigData<Configuration::Common>(Configuration::Common& obj, Configuration& config) {
  gits::CLog::SetLogLevel(config.common.shared.thresholdLogLevel);
  gits::CLog::SetLogToConsole(config.common.shared.logToConsole);

  obj.player.DeriveData(config);
  obj.recorder.DeriveData(config);
  obj.shared.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::OpenGL>(Configuration::OpenGL& obj, Configuration& config) {
  obj.player.DeriveData(config);
  obj.recorder.DeriveData(config);
  obj.shared.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::Vulkan>(Configuration::Vulkan& obj, Configuration& config) {
  obj.player.DeriveData(config);
  obj.recorder.DeriveData(config);
  obj.shared.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::DirectX>(Configuration::DirectX& obj, Configuration& config) {
  obj.player.DeriveData(config);
  obj.capture.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::OpenCL>(Configuration::OpenCL& obj, Configuration& config) {
  obj.player.DeriveData(config);
  obj.recorder.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::LevelZero>(Configuration::LevelZero& obj,
                                                Configuration& config) {
  obj.player.DeriveData(config);
  obj.recorder.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::Common::Shared>(Configuration::Common::Shared& obj,
                                                     Configuration& config) {
  auto TraceDataAll = {TraceData::FRAME_NUMBER};
  if (config.common.shared.thresholdLogLevel == LogLevel::TRACEV) {
    for (auto& opt : TraceDataAll) {
      obj.traceDataOpts.insert(opt);
    }
  }

#if defined GITS_PLATFORM_WINDOWS
  obj.libL0Driver = std::filesystem::path("ze_intel_gpu64.dll");
#elif defined GITS_PLATFORM_X11
  obj.libL0Driver = std::filesystem::path("libze_intel_gpu.so.1");
#else
  //#error Invalid platform.
  obj.libL0Driver = std::filesystem::path("ze_intel_gpu64.dll");
#endif

  if (Configurator::IsPlayer()) {
    obj.libGL = config.common.player.libGL;
    obj.libEGL = config.common.player.libEGL;
    obj.libGLESv1 = config.common.player.libGLESv1;
    obj.libGLESv2 = config.common.player.libGLESv2;
    obj.libClPath = config.common.player.libClPath;
    obj.libVK = config.common.player.libVK;
    obj.libOcloc = config.common.player.libOcloc;
    obj.libL0 = config.common.player.libL0;

    obj.useEvents = config.common.player.useEvents;
    obj.scriptArgsStr = config.common.player.scriptArgsStr;
  } else {
    obj.libGL = config.common.recorder.libGL;
    obj.libEGL = config.common.recorder.libEGL;
    obj.libGLESv1 = config.common.recorder.libGLESv1;
    obj.libGLESv2 = config.common.recorder.libGLESv2;
    obj.libClPath = config.common.recorder.libClPath;
    obj.libVK = config.common.recorder.libVK;
    obj.libOcloc = config.common.recorder.libOcloc;
    obj.libL0 = config.common.recorder.libL0;

    obj.useEvents = config.common.recorder.useEvents;
    obj.scriptArgsStr = config.common.recorder.scriptArgsStr;
  }
}
template <>
void DeriveConfigData<Configuration::Common::Player>(Configuration::Common::Player& obj,
                                                     Configuration& config) {
  if (!config.common.player.streamPath.empty()) {
    obj.streamDir = config.common.player.streamPath.parent_path();
  }

  obj.windowMode = WindowMode::NORMAL;
  if (config.common.player.fullscreen) {
    obj.windowMode = WindowMode::EXCLUSIVE_FULLSCREEN;
  }

  if (!config.common.player.eventScript.empty()) {
    auto& scriptPath = config.common.player.eventScript;
    if (!std::filesystem::exists(scriptPath) || !std::filesystem::is_regular_file(scriptPath)) {
      throw std::runtime_error("EventScript error: Could not find file: " + scriptPath.string());
    }
    gits::lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    obj.useEvents = true;
  }

  if (!config.common.player.forceWindowPosStr.empty()) {
    obj.forceWindowPos.enabled = true;
    std::pair<uint32_t, uint32_t> windowCoords;
    if (!parse_pair(config.common.player.forceWindowPosStr, windowCoords)) {
      throw std::runtime_error("Couldn't parse window position parameter.");
    }
    obj.forceWindowPos.x = windowCoords.first;
    obj.forceWindowPos.y = windowCoords.second;
  }

#if defined GITS_PLATFORM_WINDOWS
  if (!config.common.player.forceDesktopResolutionStr.empty()) {
    obj.forceDesktopResolution.enabled = true;
    std::pair<uint32_t, uint32_t> forcedDesktopResolution;
    if (!parse_pair(config.common.player.forceDesktopResolutionStr, forcedDesktopResolution)) {
      throw std::runtime_error("Couldn't parse resolution");
    }
    obj.forceDesktopResolution.width = forcedDesktopResolution.first;
    obj.forceDesktopResolution.height = forcedDesktopResolution.second;
  }
#endif

  if (!config.common.player.forceWindowSizeStr.empty()) {
    obj.forceWindowSize.enabled = true;
    std::pair<uint32_t, uint32_t> windowSize;
    if (!parse_pair(config.common.player.forceWindowSizeStr, windowSize)) {
      throw std::runtime_error("Couldn't parse window size parameter.");
    }
    obj.forceWindowSize.width = windowSize.first;
    obj.forceWindowSize.height = windowSize.second;
  }

  if (config.common.player.forceScissorVector.size() == 4) {
    obj.forceScissor.enabled = true;
    obj.forceScissor.x = config.common.player.forceScissorVector[0];
    obj.forceScissor.y = config.common.player.forceScissorVector[1];
    obj.forceScissor.width = config.common.player.forceScissorVector[2];
    obj.forceScissor.height = config.common.player.forceScissorVector[3];
  }
};

namespace {
std::string PrepareDumpPath(const std::string& str, bool uniqueDump) {
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
    processname += gits::GetWindowsProcessName(pid);
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
} // namespace

template <>
void DeriveConfigData<Configuration::Common::Recorder>(Configuration::Common::Recorder& obj,
                                                       Configuration& config) {
  if (config.common.recorder.recordingMode != RecordingMode::NONE) {
    obj.enabled = true;
  }

  if (!config.common.recorder.exitKeysStr.empty()) {
    obj.exitKeys = std::move(gits::parseKeys(config.common.recorder.exitKeysStr));
  }

  obj.dumpPath = PrepareDumpPath(config.common.recorder.dumpPath.string(),
                                 config.common.recorder.uniqueDumpDirectory);

  if (config.common.recorder.recordingMode == RecordingMode::CCODE) {
    obj.zipTextFiles = false;
  }

  if (config.common.recorder.highIntegrity) {
    LOG_INFO << "High integrity mode";
    obj.tokenBurst = 1;
    obj.tokenBurstNum = 1;
    if (config.common.recorder.zipTextFiles) {
      obj.zipTextFiles = false;
      LOG_WARNING << "High Integrity mode active - overriding ZipTextFiles to False.";
    }
    obj.compression.type = stringTo<CompressionType>("NONE");
    LOG_WARNING << "High Integrity mode active - disabling compression.";
  }

  if (!config.common.recorder.eventScript.empty()) {
    auto& scriptPath = config.common.recorder.eventScript;
    if (!std::filesystem::exists(scriptPath) || !std::filesystem::is_regular_file(scriptPath)) {
      throw std::runtime_error("EventScript error: Could not find file: " + scriptPath.string());
    }
    gits::lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    gits::CGits::Instance().ProcessLuaFunctionsRegistrators();
    obj.useEvents = true;
  }
}

template <>
void DeriveConfigData<Configuration::OpenGL::Shared>(Configuration::OpenGL::Shared& obj,
                                                     Configuration& config) {
  if (!config.opengl.shared.forceGLVersion.empty()) {
    // The version is <major_version>.<minor_version> optionally followed by additional text.
    std::regex oglVersion("([0-9]+)\\.([0-9]+).*");
    std::smatch match;

    if (std::regex_search(config.opengl.shared.forceGLVersion, match, oglVersion)) {
      auto& forceGLVersionMajor = obj.forceGLVersionMajor;
      auto& forceGLVersionMinor = obj.forceGLVersionMinor;

      // match[0] is the whole input string (forceGLVersion).
      forceGLVersionMajor = gits::stoui(match[1]);
      forceGLVersionMinor = gits::stoui(match[2]);

      LOG_INFO << "OpenGL version string override set to: " << obj.forceGLVersion;
      LOG_INFO << "OpenGL version number override set to: " << forceGLVersionMajor << "."
               << forceGLVersionMinor;
    } else {
      auto msg =
          "Incorrect OpenGL version string specified: " + config.opengl.shared.forceGLVersion +
          "\nCheck recorder config.";
      LOG_ERROR << msg;
      throw std::runtime_error((std::string)EXCEPTION_MESSAGE + " " + msg);
    }
  }
};

template <>
void DeriveConfigData<Configuration::OpenGL::Player>(Configuration::OpenGL::Player& obj,
                                                     Configuration& config) {
  if (!config.opengl.player.affectedViewport.empty()) {
    obj.affectViewport = true;
  }
};

template <>
void DeriveConfigData<Configuration::OpenGL::Recorder>(Configuration::OpenGL::Recorder& obj,
                                                       Configuration& config) {
  if (config.opengl.recorder.mode == OpenGLRecorderMode::ALL) {
    obj.frames.startFrame = 1;
    obj.frames.stopFrame = UINT_MAX;
    obj.frames.frameSeparators.glFinishSep = false;
    obj.frames.frameSeparators.glFlushSep = false;
    obj.oglSingleDraw.number = UINT_MAX;
    obj.oglDrawsRange.startDraw = UINT_MAX;
    obj.oglDrawsRange.stopDraw = UINT_MAX;
    obj.oglDrawsRange.frame = UINT_MAX;
  } else if (config.opengl.recorder.mode == OpenGLRecorderMode::FRAMES) {
    obj.all.exitFrame = UINT_MAX;
    obj.all.exitDeleteContext = 0;
    obj.oglSingleDraw.number = UINT_MAX;
    obj.oglDrawsRange.startDraw = UINT_MAX;
    obj.oglDrawsRange.stopDraw = UINT_MAX;
    obj.oglDrawsRange.frame = UINT_MAX;
    if (!config.opengl.recorder.frames.startKeysStr.empty()) {
      obj.frames.startKeys = std::move(gits::parseKeys(config.opengl.recorder.frames.startKeysStr));
    }
  } else if (config.opengl.recorder.mode == OpenGLRecorderMode::SINGLE_DRAW) {
    obj.all.exitFrame = UINT_MAX;
    obj.all.exitDeleteContext = 0;
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.frames.frameSeparators.glFinishSep = false;
    obj.frames.frameSeparators.glFlushSep = false;
    obj.oglDrawsRange.startDraw = UINT_MAX;
    obj.oglDrawsRange.stopDraw = UINT_MAX;
    obj.oglDrawsRange.frame = UINT_MAX;
  } else if (config.opengl.recorder.mode == OpenGLRecorderMode::DRAWS_RANGE) {
    obj.all.exitFrame = UINT_MAX;
    obj.all.exitDeleteContext = 0;
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.frames.frameSeparators.glFinishSep = false;
    obj.frames.frameSeparators.glFlushSep = false;
    obj.oglSingleDraw.number = UINT_MAX;
  }

  if (config.common.recorder.recordingMode == RecordingMode::CCODE &&
      config.opengl.recorder.mode != OpenGLRecorderMode::ALL &&
      !config.opengl.recorder.ccodeRangesWA) {
    LOG_ERROR << "CCodeDump is possible only if OpenGL.Capture.Mode is set to: All. So, if you for "
                 "example need one frame CCode stream please record one frame binary stream and "
                 "then recapture it to CCode.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
};

template <>
void DeriveConfigData<Configuration::Vulkan::Player>(Configuration::Vulkan::Player& obj,
                                                     Configuration& config) {
  if (!config.vulkan.player.captureVulkanRenderPasses.empty() ||
      !config.vulkan.player.captureVulkanRenderPassesResources.empty() ||
      config.vulkan.player.oneVulkanDrawPerCommandBuffer ||
      config.vulkan.player.oneVulkanRenderPassPerCommandBuffer ||
      !config.vulkan.player.captureVulkanDraws.empty() ||
      !config.vulkan.player.captureVulkanResources.empty()) {
    obj.execCmdBuffsBeforeQueueSubmit = true;
  }

  if (!config.vulkan.player.captureVulkanDraws.empty() ||
      !config.vulkan.player.captureVulkanResources.empty()) {
    obj.oneVulkanDrawPerCommandBuffer = true;
  }

  if (!config.vulkan.player.captureVulkanRenderPasses.empty() ||
      !config.vulkan.player.captureVulkanRenderPassesResources.empty()) {
    obj.oneVulkanRenderPassPerCommandBuffer = true;
  }

  if (config.vulkan.player.disableShaderGroupHandlesPatching) {
    obj.patchShaderGroupHandles = false;
  }

  if (!config.vulkan.player.renderDocCaptureFrames.empty()) {
    obj.renderDoc.mode = VkRenderDocCaptureMode::FRAMES;
    obj.renderDoc.captureRange = config.vulkan.player.renderDocCaptureFrames;
  }
  if (!config.vulkan.player.renderDocCaptureVKSubmits.empty()) {
    obj.renderDoc.mode = VkRenderDocCaptureMode::QUEUE_SUBMIT;
    obj.renderDoc.captureRange = config.vulkan.player.renderDocCaptureVKSubmits;
  }
}

namespace {
void SetRangeSpecial(Configuration& config,
                     const std::string& val,
                     const gits::VulkanObjectMode& mode,
                     size_t expectedVecSize) {
  config.vulkan.recorder.objRange.rangeSpecial.SetFromString(val);
  if (config.vulkan.recorder.objRange.rangeSpecial.objVector.size() == expectedVecSize) {
    config.vulkan.recorder.objRange.rangeSpecial.objMode = mode;
  } else {
    LOG_ERROR << "Incorrect range for mode: " << mode;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}
} // namespace

namespace {
#ifdef GITS_PLATFORM_WINDOWS
std::vector<std::string> renderDocCompatibilitySuppressedExtensions = {
    "VK_EXT_graphics_pipeline_library",
    "VK_EXT_extended_dynamic_state3",
    "VK_EXT_external_memory_host",
    "VK_KHR_map_memory2",
    "VK_EXT_dynamic_rendering_unused_attachments",
    "VK_EXT_host_image_copy",
    "VK_KHR_maintenance5"};
#endif
} // namespace

template <>
void DeriveConfigData<Configuration::Vulkan::Recorder>(Configuration::Vulkan::Recorder& obj,
                                                       Configuration& config) {
  if (config.vulkan.recorder.mode == VulkanRecorderMode::ALL) {
    obj.frames.startFrame = 1;
    obj.frames.stopFrame = UINT_MAX;
    obj.objRange.rangeSpecial.range = BitRange(false);
    obj.objRange.rangeSpecial.objMode = VulkanObjectMode::MODE_VK_NONE;
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::FRAMES) {
    obj.all.exitFrame = UINT_MAX;
    obj.objRange.rangeSpecial.range = BitRange(false);
    obj.objRange.rangeSpecial.objMode = VulkanObjectMode::MODE_VK_NONE;
    if (!config.vulkan.recorder.frames.startKeysStr.empty()) {
      obj.frames.startKeys = std::move(gits::parseKeys(config.vulkan.recorder.frames.startKeysStr));
    }
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::QUEUE_SUBMIT) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.queueSubmit.queueSubmitStr,
                    VulkanObjectMode::MODE_VK_QUEUE_SUBMIT, 0);
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::COMMAND_BUFFERS_RANGE) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.commandBuffersRange.commandBuffersRangeStr,
                    VulkanObjectMode::MODE_VK_COMMAND_BUFFER, 2);
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::RENDER_PASS_RANGE) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.renderPassRange.renderPassRangeStr,
                    VulkanObjectMode::MODE_VK_RENDER_PASS, 3);
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::DRAWS_RANGE) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.drawsRange.drawsRangeStr,
                    VulkanObjectMode::MODE_VK_DRAW, 4);
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::DISPATCH_RANGE) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.dispatchRange.dispatchRangeStr,
                    VulkanObjectMode::MODE_VK_DISPATCH, 3);
  } else if (config.vulkan.recorder.mode == VulkanRecorderMode::BLIT_RANGE) {
    obj.frames.startFrame = UINT_MAX;
    obj.frames.stopFrame = UINT_MAX;
    obj.all.exitFrame = UINT_MAX;
    SetRangeSpecial(config, config.vulkan.recorder.blitRange.blitRangeStr,
                    VulkanObjectMode::MODE_VK_BLIT, 3);
  }

  switch (config.vulkan.recorder.memoryTrackingMode) {
  case MemoryTrackingMode::SHADOW_AND_ACCESS_DETECTION:
    obj.shadowMemory = true;
    obj.memoryAccessDetection = true;
    break;
#ifdef GITS_PLATFORM_WINDOWS
  case MemoryTrackingMode::EXTERNAL:
    obj.useExternalMemoryExtension = true;
    break;
  case MemoryTrackingMode::WRITE_WATCH:
    obj.writeWatchDetection = true;
    obj.shadowMemory = true;
    break;
#endif
  case MemoryTrackingMode::FULL_MEMORY_DUMP:
    // everything is already set to false by default.
    break;
  }

#ifdef GITS_PLATFORM_WINDOWS
  if (config.vulkan.recorder.renderDocCompatibility) {
    auto& rdocSuppressExtensions = renderDocCompatibilitySuppressedExtensions;
    auto& suppressExtensions = config.vulkan.shared.suppressExtensions;
    std::copy_if(rdocSuppressExtensions.begin(), rdocSuppressExtensions.end(),
                 std::back_inserter(suppressExtensions), [suppressExtensions](std::string s) {
                   return std::find(suppressExtensions.begin(), suppressExtensions.end(), s) ==
                          suppressExtensions.end();
                 });
  }
#endif
};

template <>
void DeriveConfigData<Configuration::OpenCL::Recorder>(Configuration::OpenCL::Recorder& obj,
                                                       Configuration& config) {
  if (config.opencl.recorder.mode == OpenCLRecorderMode::SINGLE_KERNEL) {
    // WA
    config.opengl.recorder.frames.startFrame = UINT_MAX;
    obj.oclKernelsRange.startKernel = UINT_MAX;
    obj.oclKernelsRange.stopKernel = UINT_MAX;
  } else if (config.opencl.recorder.mode == OpenCLRecorderMode::KERNELS_RANGE) {
    config.opengl.recorder.frames.startFrame = UINT_MAX;
    obj.oclSingleKernel.number = UINT_MAX;
  }
  if (config.opencl.recorder.mode != OpenCLRecorderMode::ALL &&
      (config.common.recorder.removeGLSharing || config.common.recorder.removeDXSharing)) {
    LOG_ERROR << "Unsharing is not supported in subcapture mode. If you need a subcapture from "
                 "sharing stream, create unshared stream first, then do a subcapture of that.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
}

template <>
void DeriveConfigData<Configuration::LevelZero::Player>(Configuration::LevelZero::Player& obj,
                                                        Configuration& config) {
  auto parsedCaptureKernels = ParseCaptureKernels(config.levelzero.player.captureKernelsStr);
  obj.captureCommandQueues = BitRange(parsedCaptureKernels[0]);
  obj.captureCommandLists = BitRange(parsedCaptureKernels[1]);
  if (parsedCaptureKernels.size() == 2) {
    obj.captureKernels = BitRange(true);
  } else if (parsedCaptureKernels.size() == 3) {
    obj.captureKernels = BitRange(parsedCaptureKernels[2]);
  } else {
    LOG_ERROR << "Incorrect config LevelZero.Player.CaptureKernels";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  if (config.levelzero.player.captureInputKernels && config.levelzero.player.captureAfterSubmit) {
    LOG_ERROR << "CaptureInputKernels and CaptureAfterSubmit options are mutually exclusive.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
};

namespace {
void LoadLevelZeroSubcaptureSettings(Configuration& config, const std::string& kernelInfo) {
  if (!kernelInfo.empty()) {
    // WA (trigger for _captureState to be CAP_INITIATED for delaying recorder.Start())
    config.opengl.recorder.frames.startFrame = UINT_MAX;
    std::istringstream issL0ObjectsRange(kernelInfo);
    std::vector<std::string> objectsTable;
    std::string strObj;
    while (std::getline(issL0ObjectsRange, strObj, '/')) {
      objectsTable.push_back(strObj);
    }
    if (objectsTable.size() != 3) {
      LOG_ERROR << "Incorrect config LevelZero.Capture.Kernel.Range";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    auto pattern = std::regex("(\\d+)");
    auto iter = std::sregex_iterator(objectsTable[0].begin(), objectsTable[0].end(), pattern);
    auto end = std::sregex_iterator();
    auto size = std::distance(iter, end);
    auto& startCommandQueueSubmit = config.levelzero.recorder.kernel.startCommandQueueSubmit;
    auto& stopCommandQueueSubmit = config.levelzero.recorder.kernel.stopCommandQueueSubmit;
    startCommandQueueSubmit = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopCommandQueueSubmit = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopCommandQueueSubmit = startCommandQueueSubmit;
    } else {
      LOG_ERROR << "Incorrect config LevelZero.Capture.Kernel.Range command queue submission";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startCommandQueueSubmit > stopCommandQueueSubmit) {
      LOG_ERROR << "Incorrect config: LevelZero.Capture.Kernel.Range start command queue "
                   "submission can't be greater than stop command queue submission";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    iter = std::sregex_iterator(objectsTable[1].begin(), objectsTable[1].end(), pattern);
    size = std::distance(iter, end);
    auto& startCommandList = config.levelzero.recorder.kernel.startCommandList;
    auto& stopCommandList = config.levelzero.recorder.kernel.stopCommandList;
    startCommandList = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopCommandList = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopCommandList = startCommandList;
    } else {
      LOG_ERROR << "Incorrect config LevelZero.Capture.Kernel.Range command list";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    iter = std::sregex_iterator(objectsTable[2].begin(), objectsTable[2].end(), pattern);
    size = std::distance(iter, std::move(end));
    auto& startKernel = config.levelzero.recorder.kernel.startKernel;
    auto& stopKernel = config.levelzero.recorder.kernel.stopKernel;
    startKernel = (unsigned)std::stoul((*iter++)[0], nullptr);
    if (size == 2U) {
      stopKernel = (unsigned)std::stoul((*iter)[0], nullptr);
    } else if (size == 1U) {
      stopKernel = startKernel;
    } else {
      LOG_ERROR << "Incorrect config LevelZero.Capture.Kernel.Range kernel";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (startKernel > stopKernel) {
      LOG_ERROR << "In config LevelZero.Capture.Kernel.Range start kernel "
                   "greater then stop kernel is not supported";
      throw gits::EOperationFailed(EXCEPTION_MESSAGE);
    }

    config.levelzero.recorder.kernel.singleCapture =
        (startCommandQueueSubmit == stopCommandQueueSubmit) &&
        (startCommandList == stopCommandList) && (startKernel == stopKernel);
  }
}
} // namespace

template <>
void DeriveConfigData<Configuration::LevelZero::Recorder>(Configuration::LevelZero::Recorder& obj,
                                                          Configuration& config) {
  if (config.levelzero.recorder.mode == LevelZeroRecorderMode::KERNEL) {
    LoadLevelZeroSubcaptureSettings(config, config.levelzero.recorder.kernel.kernelRangeStr);
  }
  auto parsedCaptureKernels = ParseCaptureKernels(config.levelzero.recorder.dumpKernelsStr);
  obj.captureCommandQueues = BitRange(parsedCaptureKernels[0]);
  obj.captureCommandLists = BitRange(parsedCaptureKernels[1]);
  if (parsedCaptureKernels.size() == 2) {
    obj.captureKernels = BitRange(true);
  } else if (parsedCaptureKernels.size() == 3) {
    obj.captureKernels = BitRange(parsedCaptureKernels[2]);
  } else {
    LOG_ERROR << "Incorrect config LevelZero.Recorder.DumpKernels";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  if (config.levelzero.recorder.captureAfterSubmit && config.levelzero.recorder.dumpInputKernels) {
    LOG_ERROR << "DumpAfterSubmit and DumpInputKernels are mutually exclusive.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
};

} // namespace gits
