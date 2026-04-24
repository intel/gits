// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "xessService.h"
#include "log.h"

namespace gits {
namespace DirectX {

XessService::~XessService() {
  if (m_XessDll) {
    FreeLibrary(m_XessDll);
  }
  if (m_XellDll) {
    FreeLibrary(m_XellDll);
  }
  if (m_XefgDll) {
    FreeLibrary(m_XefgDll);
  }
}

bool XessService::LoadXess(std::filesystem::path path) {
  if (!m_XessDll) {
    m_XessDll = LoadLibrary(path.string().c_str());
    if (!m_XessDll) {
      LOG_ERROR << "Failed to load XeSS (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(m_XessDll);

  m_XessDispatchTable.xessGetVersion =
      reinterpret_cast<decltype(xessGetVersion)*>(GetProcAddress(m_XessDll, "xessGetVersion"));

  m_XessDispatchTable.xessGetIntelXeFXVersion =
      reinterpret_cast<decltype(xessGetIntelXeFXVersion)*>(
          GetProcAddress(m_XessDll, "xessGetIntelXeFXVersion"));

  m_XessDispatchTable.xessGetProperties = reinterpret_cast<decltype(xessGetProperties)*>(
      GetProcAddress(m_XessDll, "xessGetProperties"));

  m_XessDispatchTable.xessGetInputResolution = reinterpret_cast<decltype(xessGetInputResolution)*>(
      GetProcAddress(m_XessDll, "xessGetInputResolution"));

  m_XessDispatchTable.xessGetOptimalInputResolution =
      reinterpret_cast<decltype(xessGetOptimalInputResolution)*>(
          GetProcAddress(m_XessDll, "xessGetOptimalInputResolution"));

  m_XessDispatchTable.xessGetJitterScale = reinterpret_cast<decltype(xessGetJitterScale)*>(
      GetProcAddress(m_XessDll, "xessGetJitterScale"));

  m_XessDispatchTable.xessGetVelocityScale = reinterpret_cast<decltype(xessGetVelocityScale)*>(
      GetProcAddress(m_XessDll, "xessGetVelocityScale"));

  m_XessDispatchTable.xessDestroyContext = reinterpret_cast<decltype(xessDestroyContext)*>(
      GetProcAddress(m_XessDll, "xessDestroyContext"));

  m_XessDispatchTable.xessSetJitterScale = reinterpret_cast<decltype(xessSetJitterScale)*>(
      GetProcAddress(m_XessDll, "xessSetJitterScale"));

  m_XessDispatchTable.xessSetVelocityScale = reinterpret_cast<decltype(xessSetVelocityScale)*>(
      GetProcAddress(m_XessDll, "xessSetVelocityScale"));

  m_XessDispatchTable.xessSetExposureMultiplier =
      reinterpret_cast<decltype(xessSetExposureMultiplier)*>(
          GetProcAddress(m_XessDll, "xessSetExposureMultiplier"));

  m_XessDispatchTable.xessGetExposureMultiplier =
      reinterpret_cast<decltype(xessGetExposureMultiplier)*>(
          GetProcAddress(m_XessDll, "xessGetExposureMultiplier"));

  m_XessDispatchTable.xessIsOptimalDriver = reinterpret_cast<decltype(xessIsOptimalDriver)*>(
      GetProcAddress(m_XessDll, "xessIsOptimalDriver"));

  m_XessDispatchTable.xessForceLegacyScaleFactors =
      reinterpret_cast<decltype(xessForceLegacyScaleFactors)*>(
          GetProcAddress(m_XessDll, "xessForceLegacyScaleFactors"));

  m_XessDispatchTable.xessSetLoggingCallback = reinterpret_cast<decltype(xessSetLoggingCallback)*>(
      GetProcAddress(m_XessDll, "xessSetLoggingCallback"));

  m_XessDispatchTable.xessD3D12CreateContext = reinterpret_cast<decltype(xessD3D12CreateContext)*>(
      GetProcAddress(m_XessDll, "xessD3D12CreateContext"));

  m_XessDispatchTable.xessD3D12BuildPipelines =
      reinterpret_cast<decltype(xessD3D12BuildPipelines)*>(
          GetProcAddress(m_XessDll, "xessD3D12BuildPipelines"));

  m_XessDispatchTable.xessD3D12Init =
      reinterpret_cast<decltype(xessD3D12Init)*>(GetProcAddress(m_XessDll, "xessD3D12Init"));

  m_XessDispatchTable.xessD3D12GetInitParams = reinterpret_cast<decltype(xessD3D12GetInitParams)*>(
      GetProcAddress(m_XessDll, "xessD3D12GetInitParams"));

  m_XessDispatchTable.xessD3D12Execute =
      reinterpret_cast<decltype(xessD3D12Execute)*>(GetProcAddress(m_XessDll, "xessD3D12Execute"));

  m_XessDispatchTable.xessSetMaxResponsiveMaskValue =
      reinterpret_cast<decltype(xessSetMaxResponsiveMaskValue)*>(
          GetProcAddress(m_XessDll, "xessSetMaxResponsiveMaskValue"));

  m_XessDispatchTable.xessGetMaxResponsiveMaskValue =
      reinterpret_cast<decltype(xessGetMaxResponsiveMaskValue)*>(
          GetProcAddress(m_XessDll, "xessGetMaxResponsiveMaskValue"));

  m_XessDispatchTable.xessGetPipelineBuildStatus =
      reinterpret_cast<decltype(xessGetPipelineBuildStatus)*>(
          GetProcAddress(m_XessDll, "xessGetPipelineBuildStatus"));

  xess_version_t version = {};
  m_XessDispatchTable.xessGetVersion(&version);
  LOG_INFO << "Loaded XeSS (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

bool XessService::LoadXell(std::filesystem::path path) {
  if (!m_XellDll) {
    m_XellDll = LoadLibrary(path.string().c_str());
    if (!m_XellDll) {
      LOG_ERROR << "Failed to load XeLL (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(m_XellDll);

  m_XellDispatchTable.xellDestroyContext = reinterpret_cast<decltype(xellDestroyContext)*>(
      GetProcAddress(m_XellDll, "xellDestroyContext"));
  m_XellDispatchTable.xellSetSleepMode =
      reinterpret_cast<decltype(xellSetSleepMode)*>(GetProcAddress(m_XellDll, "xellSetSleepMode"));
  m_XellDispatchTable.xellGetSleepMode =
      reinterpret_cast<decltype(xellGetSleepMode)*>(GetProcAddress(m_XellDll, "xellGetSleepMode"));
  m_XellDispatchTable.xellSleep =
      reinterpret_cast<decltype(xellSleep)*>(GetProcAddress(m_XellDll, "xellSleep"));
  m_XellDispatchTable.xellAddMarkerData = reinterpret_cast<decltype(xellAddMarkerData)*>(
      GetProcAddress(m_XellDll, "xellAddMarkerData"));
  m_XellDispatchTable.xellGetVersion =
      reinterpret_cast<decltype(xellGetVersion)*>(GetProcAddress(m_XellDll, "xellGetVersion"));
  m_XellDispatchTable.xellSetLoggingCallback = reinterpret_cast<decltype(xellSetLoggingCallback)*>(
      GetProcAddress(m_XellDll, "xellSetLoggingCallback"));
  m_XellDispatchTable.xellGetFramesReports = reinterpret_cast<decltype(xellGetFramesReports)*>(
      GetProcAddress(m_XellDll, "xellGetFramesReports"));
  m_XellDispatchTable.xellD3D12CreateContext = reinterpret_cast<decltype(xellD3D12CreateContext)*>(
      GetProcAddress(m_XellDll, "xellD3D12CreateContext"));

  xell_version_t version = {};
  m_XellDispatchTable.xellGetVersion(&version);
  LOG_INFO << "Loaded XeLL (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

bool XessService::LoadXefg(std::filesystem::path path) {
  if (!m_XefgDll) {
    m_XefgDll = LoadLibrary(path.string().c_str());
    if (!m_XefgDll) {
      LOG_ERROR << "Failed to load XeSS FG (" << path.string() << "). Playback issues may occur.";
      return false;
    }
  }
  GITS_ASSERT(m_XefgDll);

  m_XefgDispatchTable.xefgSwapChainGetVersion =
      reinterpret_cast<decltype(xefgSwapChainGetVersion)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetVersion"));
  m_XefgDispatchTable.xefgSwapChainGetProperties =
      reinterpret_cast<decltype(xefgSwapChainGetProperties)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetProperties"));
  m_XefgDispatchTable.xefgSwapChainTagFrameConstants =
      reinterpret_cast<decltype(xefgSwapChainTagFrameConstants)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainTagFrameConstants"));
  m_XefgDispatchTable.xefgSwapChainSetEnabled =
      reinterpret_cast<decltype(xefgSwapChainSetEnabled)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetEnabled"));
  m_XefgDispatchTable.xefgSwapChainSetPresentId =
      reinterpret_cast<decltype(xefgSwapChainSetPresentId)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetPresentId"));
  m_XefgDispatchTable.xefgSwapChainGetLastPresentStatus =
      reinterpret_cast<decltype(xefgSwapChainGetLastPresentStatus)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetLastPresentStatus"));
  m_XefgDispatchTable.xefgSwapChainSetLoggingCallback =
      reinterpret_cast<decltype(xefgSwapChainSetLoggingCallback)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetLoggingCallback"));
  m_XefgDispatchTable.xefgSwapChainDestroy = reinterpret_cast<decltype(xefgSwapChainDestroy)*>(
      GetProcAddress(m_XefgDll, "xefgSwapChainDestroy"));
  m_XefgDispatchTable.xefgSwapChainSetLatencyReduction =
      reinterpret_cast<decltype(xefgSwapChainSetLatencyReduction)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetLatencyReduction"));
  m_XefgDispatchTable.xefgSwapChainSetSceneChangeThreshold =
      reinterpret_cast<decltype(xefgSwapChainSetSceneChangeThreshold)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetSceneChangeThreshold"));
  m_XefgDispatchTable.xefgSwapChainGetPipelineBuildStatus =
      reinterpret_cast<decltype(xefgSwapChainGetPipelineBuildStatus)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetPipelineBuildStatus"));
  m_XefgDispatchTable.xefgSwapChainD3D12CreateContext =
      reinterpret_cast<decltype(xefgSwapChainD3D12CreateContext)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12CreateContext"));
  m_XefgDispatchTable.xefgSwapChainD3D12BuildPipelines =
      reinterpret_cast<decltype(xefgSwapChainD3D12BuildPipelines)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12BuildPipelines"));
  m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChain =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChain)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12InitFromSwapChain"));
  m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChainDesc =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChainDesc)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12InitFromSwapChainDesc"));
  m_XefgDispatchTable.xefgSwapChainD3D12GetSwapChainPtr =
      reinterpret_cast<decltype(xefgSwapChainD3D12GetSwapChainPtr)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12GetSwapChainPtr"));
  m_XefgDispatchTable.xefgSwapChainD3D12TagFrameResource =
      reinterpret_cast<decltype(xefgSwapChainD3D12TagFrameResource)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12TagFrameResource"));
  m_XefgDispatchTable.xefgSwapChainD3D12SetDescriptorHeap =
      reinterpret_cast<decltype(xefgSwapChainD3D12SetDescriptorHeap)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12SetDescriptorHeap"));
  m_XefgDispatchTable.xefgSwapChainEnableDebugFeature =
      reinterpret_cast<decltype(xefgSwapChainEnableDebugFeature)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainEnableDebugFeature"));

  xefg_swapchain_version_t version{};
  xefg_swapchain_result_t result = m_XefgDispatchTable.xefgSwapChainGetVersion(&version);
  GITS_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);
  LOG_INFO << "Loaded XeSS FG (version " << version.major << "." << version.minor << "."
           << version.patch << ")";

  return true;
}

} // namespace DirectX
} // namespace gits
