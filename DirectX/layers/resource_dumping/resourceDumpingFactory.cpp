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
#include "rootSignatureDumpLayer.h"
#include "gits.h"
#include "log.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

ResourceDumpingFactory::ResourceDumpingFactory() {
  if (Configurator::Get().directx.features.screenshots.enabled ||
      !Configurator::Get().common.player.captureFrames.empty()) {
    screenshotsLayer_ = std::make_unique<ScreenshotsLayer>();
  }
  if (Configurator::Get().directx.features.resourcesDump.enabled) {
    resourceDumpLayer_ = std::make_unique<ResourceDumpLayer>();
  }
  if (Configurator::Get().directx.features.renderTargetsDump.enabled) {
    renderTargetsDumpLayer_ = std::make_unique<RenderTargetsDumpLayer>();
  }
  if (Configurator::Get().directx.features.raytracingDump.blases) {
    if (Configurator::Get().directx.player.debugLayer) {
      accelerationStructuresDumpLayer_ = std::make_unique<AccelerationStructuresDumpLayer>();
    } else {
      LOG_ERROR << "Dumping acceleration structures demands directx debug layer turned on.";
    }
  }
  if (Configurator::Get().directx.features.rootSignatureDump.enabled) {
    rootSignatureDumpLayer_ = std::make_unique<RootSignatureDumpLayer>();
  }
}

} // namespace DirectX
} // namespace gits
