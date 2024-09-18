// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

#include <thread>
#include <mutex>

void PrePostDisableVulkan();

namespace gits {
namespace Vulkan {

IRecorderWrapper* CGitsPluginVulkan::_recorderWrapper;
std::unique_ptr<CGitsLoader> CGitsPluginVulkan::_loader;
std::mutex CGitsPluginVulkan::_mutex;
std::atomic<bool> CGitsPluginVulkan::_recorderFinished{false};

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

  try {
    std::unique_lock<std::mutex> lock(_mutex);
    if (initialized) {
      return;
    }

    _loader.reset(new CGitsLoader("GITSRecorderVulkan"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->GetRecorderWrapperPtr();

    if (!_loader->GetConfiguration().common.recorder.enabled) {
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
  std::unique_lock<std::mutex> lock(_mutex);
  _loader->ProcessTerminationDetected();
}

const Config& CGitsPluginVulkan::Configuration() {
  std::unique_lock<std::mutex> lock(_mutex);
  return _loader->GetConfiguration();
}
} // namespace Vulkan
} // namespace gits
