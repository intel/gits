// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityFactory.h"
#include "portabilityLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

PortabilityFactory::PortabilityFactory() {
  if (Config::IsRecorder()) {
    portabilityLayer_ = std::make_unique<PortabilityLayer>();
  } else if (Config::IsPlayer()) {
    if (Config::Get().directx.player.portability.resourcePlacement != "none" ||
        Config::Get().directx.player.portability.portabilityChecks) {
      portabilityLayer_ = std::make_unique<PortabilityLayer>();
    }
  }
}

} // namespace DirectX
} // namespace gits
