// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "skipCallsLayerGroup.h"
#include "configurationLib.h"
#include "skipCallsOnConfigLayerAuto.h"
#include "skipCallsOnResultLayerAuto.h"

namespace gits {
namespace DirectX {

void SkipCallsLayerGroup::loadLayers() {
  if (Configurator::Get().directx.features.skipCalls.enabled) {
    addLayer(std::make_unique<SkipCallsOnConfigLayer>());
  }
  addLayer(std::make_unique<SkipCallsOnResultLayer>());
}

} // namespace DirectX
} // namespace gits
