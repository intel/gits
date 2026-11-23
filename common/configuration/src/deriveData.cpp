// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "log.h"

namespace gits {
template <>
void DeriveConfigData<Configuration>(Configuration& obj, Configuration& config) {
  obj.common.DeriveData(config);
  obj.opengl.DeriveData(config);
  obj.vulkan.DeriveData(config);
  obj.directx.DeriveData(config);
}

template <>
void DeriveConfigData<Configuration::Common>(Configuration::Common& obj, Configuration& config) {
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
  obj.recorder.DeriveData(config);
};

template <>
void DeriveConfigData<Configuration::Common::Shared>(Configuration::Common::Shared& obj,
                                                     Configuration& config) {
  if (Configurator::IsPlayer()) {
    obj.libGL = config.common.player.libGL;
    obj.libEGL = config.common.player.libEGL;
    obj.libGLESv1 = config.common.player.libGLESv1;
    obj.libGLESv2 = config.common.player.libGLESv2;
    // We derive the value of the Common.Shared.LogToConsole only if the deprecated option wasn't used
    if (config.common.shared.logToConsole.has_value()) {
      LOG_WARNING << "Option Common.Shared.LogToConsole is deprecated, please use the player "
                     "(Common.Player.LogToConsole) specific option";
      config.common.player.logToConsole = config.common.shared.logToConsole.value();
    } else {
      obj.logToConsole = config.common.player.logToConsole;
    }
    obj.useEvents = config.common.player.useEvents;
    obj.scriptArgsStr = config.common.player.scriptArgsStr;
  } else {
    obj.libGL = config.common.recorder.libGL;
    obj.libEGL = config.common.recorder.libEGL;
    obj.libGLESv1 = config.common.recorder.libGLESv1;
    obj.libGLESv2 = config.common.recorder.libGLESv2;
    // We derive the value of the Common.Shared.LogToConsole only if the deprecated option wasn't used
    if (config.common.shared.logToConsole.has_value()) {
      LOG_WARNING << "Option Common.Shared.LogToConsole is deprecated, please use the recorder "
                     "(Common.Recorder.LogToConsole) specific option";
      config.common.recorder.logToConsole = config.common.shared.logToConsole.value();
    } else {
      obj.logToConsole = config.common.recorder.logToConsole;
    }
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
  if (!config.common.recorder.exitKeysStr.empty()) {
    obj.exitKeys = gits::parseKeys(config.common.recorder.exitKeysStr);
  }

  obj.dumpPath = PrepareDumpPath(config.common.recorder.dumpPath.string(),
                                 config.common.recorder.uniqueDumpDirectory);

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

  // Deprecated option handling
  // Since CCode was removed, only the binary mode leads to recording being enabled
  if (config.common.recorder.recordingMode.has_value()) {
    switch (config.common.recorder.recordingMode.value()) {
    case RecordingMode::BINARY:
      LOG_WARNING << "RecordingMode option is deprecated";
      LOG_WARNING << "Recording will be enabled regardless";
      LOG_WARNING << "Please use the Common.Recorder.Enabled option set to true";
      obj.enabled = true;
      break;
    case RecordingMode::CCODE:
      LOG_WARNING
          << "RecordingMode option is deprecated and CCode recording is no longer available";
      LOG_WARNING << "Recording will be disabled";
      obj.enabled = false;
      break;
    case RecordingMode::NONE:
      LOG_WARNING << "RecordingMode option is deprecated";
      LOG_WARNING << "Recording will be disabled regardless";
      LOG_WARNING << "Please use the Common.Recorder.Enabled option set to false";
      obj.enabled = false;
      break;
    default:
      break;
    }
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
      obj.frames.startKeys = gits::parseKeys(config.opengl.recorder.frames.startKeysStr);
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
};

namespace {
#ifdef GITS_PLATFORM_WINDOWS
std::vector<std::string> renderDocCompatibilitySuppressedExtensions = {
    "VK_EXT_external_memory_host"};
#endif
} // namespace

template <>
void DeriveConfigData<Configuration::Vulkan::Recorder>(Configuration::Vulkan::Recorder& obj,
                                                       Configuration& config) {
#ifdef GITS_PLATFORM_WINDOWS
  if (Configurator::IsRecorder() && config.vulkan.recorder.renderDocCompatibility) {
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

} // namespace gits
