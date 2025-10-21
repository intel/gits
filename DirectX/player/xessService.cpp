// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

} // namespace DirectX
} // namespace gits
