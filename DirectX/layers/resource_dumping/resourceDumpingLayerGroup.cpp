// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpingLayerGroup.h"
#include "screenshotsLayer.h"
#include "resourceDumpLayerAuto.h"
#include "renderTargetsDumpLayer.h"
#include "dispatchOutputsDumpLayer.h"
#include "accelerationStructuresDumpLayer.h"
#include "rootSignatureDumpLayer.h"
#include "log.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

void ResourceDumpingLayerGroup::loadLayers() {
  if (Configurator::Get().directx.features.screenshots.enabled ||
      !Configurator::Get().common.player.captureFrames.empty()) {
    addLayer(std::make_unique<ScreenshotsLayer>());
  }
  if (Configurator::Get().directx.features.resourcesDump.enabled) {
    addLayer(std::make_unique<ResourceDumpLayer>());
  }
  if (Configurator::Get().directx.features.renderTargetsDump.enabled) {
    addLayer(std::make_unique<RenderTargetsDumpLayer>());
  }
  if (Configurator::Get().directx.features.dispatchOutputsDump.enabled) {
    addLayer(std::make_unique<DispatchOutputsDumpLayer>());
  }
  if (Configurator::Get().directx.features.raytracingDump.blases) {
    if (Configurator::Get().directx.player.debugLayer) {
      addLayer(std::make_unique<AccelerationStructuresDumpLayer>());
    } else {
      LOG_ERROR << "Dumping acceleration structures demands directx debug layer turned on.";
    }
  }
  if (Configurator::Get().directx.features.rootSignatureDump.enabled) {
    addLayer(std::make_unique<RootSignatureDumpLayer>());
  }
}

} // namespace DirectX
} // namespace gits
