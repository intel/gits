// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayerGroup.h"
#include "portabilityLayer.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

void PortabilityLayerGroup::loadLayers() {
  if (Configurator::IsRecorder()) {
    addLayer(std::make_unique<PortabilityLayer>());
  } else if (Configurator::IsPlayer()) {
    auto& playerConfig = Configurator::Get().directx.player;
    if (playerConfig.execute && (playerConfig.portability.resourcePlacement != "none" ||
                                 playerConfig.portability.portabilityChecks ||
                                 playerConfig.portability.portabilityAssertions)) {
      addLayer(std::make_unique<PortabilityLayer>());
    }
  }
}

} // namespace DirectX
} // namespace gits
