// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "xessService.h"
#include "log.h"

namespace gits {
namespace DirectX {

XessService::~XessService() {
  if (xessDll_) {
    FreeLibrary(xessDll_);
  }
  if (xellDll_) {
    FreeLibrary(xellDll_);
  }
  if (xefgDll_) {
    FreeLibrary(xefgDll_);
  }
}

bool XessService::loadXess(std::filesystem::path path) {
  if (!xessDll_) {
    xessDll_ = LoadLibrary(path.string().c_str());
    if (!xessDll_) {
      LOG_ERROR << "Failed to load XeSS (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(xessDll_);

  xessDispatchTable_.xessGetVersion =
      reinterpret_cast<decltype(xessGetVersion)*>(GetProcAddress(xessDll_, "xessGetVersion"));

  xessDispatchTable_.xessGetIntelXeFXVersion = reinterpret_cast<decltype(xessGetIntelXeFXVersion)*>(
      GetProcAddress(xessDll_, "xessGetIntelXeFXVersion"));

  xessDispatchTable_.xessGetProperties =
      reinterpret_cast<decltype(xessGetProperties)*>(GetProcAddress(xessDll_, "xessGetProperties"));

  xessDispatchTable_.xessGetInputResolution = reinterpret_cast<decltype(xessGetInputResolution)*>(
      GetProcAddress(xessDll_, "xessGetInputResolution"));

  xessDispatchTable_.xessGetOptimalInputResolution =
      reinterpret_cast<decltype(xessGetOptimalInputResolution)*>(
          GetProcAddress(xessDll_, "xessGetOptimalInputResolution"));

  xessDispatchTable_.xessGetJitterScale = reinterpret_cast<decltype(xessGetJitterScale)*>(
      GetProcAddress(xessDll_, "xessGetJitterScale"));

  xessDispatchTable_.xessGetVelocityScale = reinterpret_cast<decltype(xessGetVelocityScale)*>(
      GetProcAddress(xessDll_, "xessGetVelocityScale"));

  xessDispatchTable_.xessDestroyContext = reinterpret_cast<decltype(xessDestroyContext)*>(
      GetProcAddress(xessDll_, "xessDestroyContext"));

  xessDispatchTable_.xessSetJitterScale = reinterpret_cast<decltype(xessSetJitterScale)*>(
      GetProcAddress(xessDll_, "xessSetJitterScale"));

  xessDispatchTable_.xessSetVelocityScale = reinterpret_cast<decltype(xessSetVelocityScale)*>(
      GetProcAddress(xessDll_, "xessSetVelocityScale"));

  xessDispatchTable_.xessSetExposureMultiplier =
      reinterpret_cast<decltype(xessSetExposureMultiplier)*>(
          GetProcAddress(xessDll_, "xessSetExposureMultiplier"));

  xessDispatchTable_.xessGetExposureMultiplier =
      reinterpret_cast<decltype(xessGetExposureMultiplier)*>(
          GetProcAddress(xessDll_, "xessGetExposureMultiplier"));

  xessDispatchTable_.xessIsOptimalDriver = reinterpret_cast<decltype(xessIsOptimalDriver)*>(
      GetProcAddress(xessDll_, "xessIsOptimalDriver"));

  xessDispatchTable_.xessForceLegacyScaleFactors =
      reinterpret_cast<decltype(xessForceLegacyScaleFactors)*>(
          GetProcAddress(xessDll_, "xessForceLegacyScaleFactors"));

  xessDispatchTable_.xessSetLoggingCallback = reinterpret_cast<decltype(xessSetLoggingCallback)*>(
      GetProcAddress(xessDll_, "xessSetLoggingCallback"));

  xessDispatchTable_.xessD3D12CreateContext = reinterpret_cast<decltype(xessD3D12CreateContext)*>(
      GetProcAddress(xessDll_, "xessD3D12CreateContext"));

  xessDispatchTable_.xessD3D12BuildPipelines = reinterpret_cast<decltype(xessD3D12BuildPipelines)*>(
      GetProcAddress(xessDll_, "xessD3D12BuildPipelines"));

  xessDispatchTable_.xessD3D12Init =
      reinterpret_cast<decltype(xessD3D12Init)*>(GetProcAddress(xessDll_, "xessD3D12Init"));

  xessDispatchTable_.xessD3D12GetInitParams = reinterpret_cast<decltype(xessD3D12GetInitParams)*>(
      GetProcAddress(xessDll_, "xessD3D12GetInitParams"));

  xessDispatchTable_.xessD3D12Execute =
      reinterpret_cast<decltype(xessD3D12Execute)*>(GetProcAddress(xessDll_, "xessD3D12Execute"));

  xessDispatchTable_.xessSetMaxResponsiveMaskValue =
      reinterpret_cast<decltype(xessSetMaxResponsiveMaskValue)*>(
          GetProcAddress(xessDll_, "xessSetMaxResponsiveMaskValue"));

  xessDispatchTable_.xessGetMaxResponsiveMaskValue =
      reinterpret_cast<decltype(xessGetMaxResponsiveMaskValue)*>(
          GetProcAddress(xessDll_, "xessGetMaxResponsiveMaskValue"));

  xessDispatchTable_.xessGetPipelineBuildStatus =
      reinterpret_cast<decltype(xessGetPipelineBuildStatus)*>(
          GetProcAddress(xessDll_, "xessGetPipelineBuildStatus"));

  xess_version_t version = {};
  xessDispatchTable_.xessGetVersion(&version);
  LOG_INFO << "Loaded XeSS (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

bool XessService::loadXell(std::filesystem::path path) {
  if (!xellDll_) {
    xellDll_ = LoadLibrary(path.string().c_str());
    if (!xellDll_) {
      LOG_ERROR << "Failed to load XeLL (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(xellDll_);

  xellDispatchTable_.xellDestroyContext = reinterpret_cast<decltype(xellDestroyContext)*>(
      GetProcAddress(xellDll_, "xellDestroyContext"));
  xellDispatchTable_.xellSetSleepMode =
      reinterpret_cast<decltype(xellSetSleepMode)*>(GetProcAddress(xellDll_, "xellSetSleepMode"));
  xellDispatchTable_.xellGetSleepMode =
      reinterpret_cast<decltype(xellGetSleepMode)*>(GetProcAddress(xellDll_, "xellGetSleepMode"));
  xellDispatchTable_.xellSleep =
      reinterpret_cast<decltype(xellSleep)*>(GetProcAddress(xellDll_, "xellSleep"));
  xellDispatchTable_.xellAddMarkerData =
      reinterpret_cast<decltype(xellAddMarkerData)*>(GetProcAddress(xellDll_, "xellAddMarkerData"));
  xellDispatchTable_.xellGetVersion =
      reinterpret_cast<decltype(xellGetVersion)*>(GetProcAddress(xellDll_, "xellGetVersion"));
  xellDispatchTable_.xellSetLoggingCallback = reinterpret_cast<decltype(xellSetLoggingCallback)*>(
      GetProcAddress(xellDll_, "xellSetLoggingCallback"));
  xellDispatchTable_.xellGetFramesReports = reinterpret_cast<decltype(xellGetFramesReports)*>(
      GetProcAddress(xellDll_, "xellGetFramesReports"));
  xellDispatchTable_.xellD3D12CreateContext = reinterpret_cast<decltype(xellD3D12CreateContext)*>(
      GetProcAddress(xellDll_, "xellD3D12CreateContext"));

  xell_version_t version = {};
  xellDispatchTable_.xellGetVersion(&version);
  LOG_INFO << "Loaded XeLL (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

bool XessService::loadXefg(std::filesystem::path path) {
  if (!xefgDll_) {
    xefgDll_ = LoadLibrary(path.string().c_str());
    if (!xefgDll_) {
      LOG_ERROR << "Failed to load XeSS FG (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(xefgDll_);

  xefgDispatchTable_.xefgSwapChainGetVersion = reinterpret_cast<decltype(xefgSwapChainGetVersion)*>(
      GetProcAddress(xefgDll_, "xefgSwapChainGetVersion"));
  xefgDispatchTable_.xefgSwapChainGetProperties =
      reinterpret_cast<decltype(xefgSwapChainGetProperties)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainGetProperties"));
  xefgDispatchTable_.xefgSwapChainTagFrameConstants =
      reinterpret_cast<decltype(xefgSwapChainTagFrameConstants)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainTagFrameConstants"));
  xefgDispatchTable_.xefgSwapChainSetEnabled = reinterpret_cast<decltype(xefgSwapChainSetEnabled)*>(
      GetProcAddress(xefgDll_, "xefgSwapChainSetEnabled"));
  xefgDispatchTable_.xefgSwapChainSetPresentId =
      reinterpret_cast<decltype(xefgSwapChainSetPresentId)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainSetPresentId"));
  xefgDispatchTable_.xefgSwapChainGetLastPresentStatus =
      reinterpret_cast<decltype(xefgSwapChainGetLastPresentStatus)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainGetLastPresentStatus"));
  xefgDispatchTable_.xefgSwapChainSetLoggingCallback =
      reinterpret_cast<decltype(xefgSwapChainSetLoggingCallback)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainSetLoggingCallback"));
  xefgDispatchTable_.xefgSwapChainDestroy = reinterpret_cast<decltype(xefgSwapChainDestroy)*>(
      GetProcAddress(xefgDll_, "xefgSwapChainDestroy"));
  xefgDispatchTable_.xefgSwapChainSetLatencyReduction =
      reinterpret_cast<decltype(xefgSwapChainSetLatencyReduction)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainSetLatencyReduction"));
  xefgDispatchTable_.xefgSwapChainSetSceneChangeThreshold =
      reinterpret_cast<decltype(xefgSwapChainSetSceneChangeThreshold)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainSetSceneChangeThreshold"));
  xefgDispatchTable_.xefgSwapChainGetPipelineBuildStatus =
      reinterpret_cast<decltype(xefgSwapChainGetPipelineBuildStatus)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainGetPipelineBuildStatus"));
  xefgDispatchTable_.xefgSwapChainD3D12CreateContext =
      reinterpret_cast<decltype(xefgSwapChainD3D12CreateContext)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12CreateContext"));
  xefgDispatchTable_.xefgSwapChainD3D12BuildPipelines =
      reinterpret_cast<decltype(xefgSwapChainD3D12BuildPipelines)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12BuildPipelines"));
  xefgDispatchTable_.xefgSwapChainD3D12InitFromSwapChain =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChain)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12InitFromSwapChain"));
  xefgDispatchTable_.xefgSwapChainD3D12InitFromSwapChainDesc =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChainDesc)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12InitFromSwapChainDesc"));
  xefgDispatchTable_.xefgSwapChainD3D12GetSwapChainPtr =
      reinterpret_cast<decltype(xefgSwapChainD3D12GetSwapChainPtr)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12GetSwapChainPtr"));
  xefgDispatchTable_.xefgSwapChainD3D12TagFrameResource =
      reinterpret_cast<decltype(xefgSwapChainD3D12TagFrameResource)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12TagFrameResource"));
  xefgDispatchTable_.xefgSwapChainD3D12SetDescriptorHeap =
      reinterpret_cast<decltype(xefgSwapChainD3D12SetDescriptorHeap)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainD3D12SetDescriptorHeap"));
  xefgDispatchTable_.xefgSwapChainEnableDebugFeature =
      reinterpret_cast<decltype(xefgSwapChainEnableDebugFeature)*>(
          GetProcAddress(xefgDll_, "xefgSwapChainEnableDebugFeature"));

  xefg_swapchain_version_t version{};
  xefg_swapchain_result_t result = xefgDispatchTable_.xefgSwapChainGetVersion(&version);
  GITS_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);
  LOG_INFO << "Loaded XeSS FG (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

} // namespace DirectX
} // namespace gits
