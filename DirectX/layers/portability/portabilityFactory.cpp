// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityFactory.h"
#include "portabilityLayer.h"
#include "gits.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

PortabilityFactory::PortabilityFactory() {
  if (Configurator::IsRecorder()) {
    portabilityLayer_ = std::make_unique<PortabilityLayer>();
  } else if (Configurator::IsPlayer()) {
    if (Configurator::Get().directx.player.portability.resourcePlacement != "none" ||
        Configurator::Get().directx.player.portability.portabilityChecks ||
        Configurator::Get().directx.player.portability.portabilityAssertions) {
      portabilityLayer_ = std::make_unique<PortabilityLayer>();
    }
  }
}

} // namespace DirectX
} // namespace gits
