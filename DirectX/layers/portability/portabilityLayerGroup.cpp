// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayerGroup.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

void PortabilityLayerGroup::LoadLayers() {
  LoadLayers(nullptr);
}

void PortabilityLayerGroup::LoadLayers(
    PortabilityLayer::ResourceRegistrationCallback registerResource) {
  if (Configurator::IsRecorder()) {
    AddLayer(std::make_unique<PortabilityLayer>());
  } else if (Configurator::IsPlayer()) {
    auto& playerConfig = Configurator::Get().directx.player;
    if (playerConfig.execute && (playerConfig.portability.resourcePlacement != "none" ||
                                 playerConfig.portability.portabilityChecks ||
                                 playerConfig.portability.portabilityAssertions ||
                                 playerConfig.portability.forcePlacedToCommittedResources)) {
      if (registerResource) {
        AddLayer(std::make_unique<PortabilityLayer>(std::move(registerResource)));
      } else {
        AddLayer(std::make_unique<PortabilityLayer>());
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
