// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginVulkan.cpp
*
* @brief
*/

#include "gitsPluginVulkan.h"
#include "vulkanRecorderWrapperIface.h"

DISABLE_WARNINGS
#include <boost/thread.hpp>
ENABLE_WARNINGS

void PrePostDisableVulkan();

namespace gits {
namespace Vulkan {

IRecorderWrapper* CGitsPluginVulkan::_recorderWrapper;
std::unique_ptr<CGitsLoader> CGitsPluginVulkan::_loader;
boost::mutex CGitsPluginVulkan::_mutex;
bool CGitsPluginVulkan::_recorderFinished = false;

namespace {
void fast_exit(int code) {
#if defined GITS_PLATFORM_WINDOWS
  _exit(code);
#else
  _Exit(code);
#endif
}
} // namespace

void CGitsPluginVulkan::Initialize() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  // Sleep on initialization to allow for easier attaching to the process
  // during debugging.
  if (getenv("GITS_SLEEP")) {
    boost::this_thread::sleep(boost::posix_time::millisec(10000));
  }

  try {
    boost::unique_lock<boost::mutex> lock(_mutex);
    if (initialized) {
      return;
    }

    const char* envConfigPath = getenv("GITS_CONFIG_DIR");

    std::filesystem::path libPath = dl::this_library_path();
    std::filesystem::path configPath = libPath.parent_path();

    if (envConfigPath) {
      configPath = std::filesystem::path(envConfigPath);
    }

    _loader.reset(new CGitsLoader(configPath, "GITSRecorderVulkan"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->RecorderWrapperPtr();

    if (!_loader->Configuration().recorder.basic.enabled) {
      PrePostDisableVulkan();
    }

    CGitsPluginVulkan::_recorderWrapper->StreamFinishedEvent(PrePostDisableVulkan);

#ifdef BUILD_FOR_VULKAN_INTERCEPTOR
    _recorderWrapper->SetDriverMode(CVkDriver::DriverMode::INTERCEPTOR);
#elif BUILD_FOR_VULKAN_LAYER
    _recorderWrapper->SetDriverMode(CVkDriver::DriverMode::LAYER);
#endif

    initialized = true;
  } catch (const Exception& ex) {
    Log(ERR) << "Unhandled GITS exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (const std::exception& ex) {
    Log(ERR) << "Unhandled STD exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (...) {
    Log(ERR) << "Unhandled Unknown exception caught";
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  }
}

void CGitsPluginVulkan::ProcessTerminationDetected() {
  _loader->ProcessTerminationDetected();
}

const Config& CGitsPluginVulkan::Configuration() {
  return _loader->Configuration();
}
} // namespace Vulkan
} // namespace gits
