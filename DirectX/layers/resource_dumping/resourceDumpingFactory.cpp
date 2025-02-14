// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpingFactory.h"
#include "screenshotsLayer.h"
#include "resourceDumpLayerAuto.h"
#include "renderTargetsDumpLayer.h"
#include "accelerationStructuresDumpLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

ResourceDumpingFactory::ResourceDumpingFactory() {
  if (Config::Get().directx.features.screenshots.enabled ||
      !Config::Get().common.player.captureFrames.empty()) {
    screenshotsLayer_ = std::make_unique<ScreenshotsLayer>();
  }
  if (Config::Get().directx.features.resourcesDump.enabled) {
    resourceDumpLayer_ = std::make_unique<ResourceDumpLayer>();
  }
  if (Config::Get().directx.features.renderTargetsDump.enabled) {
    renderTargetsDumpLayer_ = std::make_unique<RenderTargetsDumpLayer>();
  }
  if (Config::Get().directx.features.raytracingDump.blases) {
    if (Config::Get().directx.player.debugLayer) {
      accelerationStructuresDumpLayer_ = std::make_unique<AccelerationStructuresDumpLayer>();
    } else {
      Log(ERR) << "Dumping acceleration structures demands directx debug layer turned on.";
    }
  }
}

} // namespace DirectX
} // namespace gits
