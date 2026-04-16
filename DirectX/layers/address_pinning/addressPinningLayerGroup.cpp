// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningLayerGroup.h"
#include "configurationLib.h"

#include "addressPinningUseLayer.h"
#include "addressPinningStoreLayer.h"

namespace gits {
namespace DirectX {

void AddressPinningLayerGroup::loadLayers() {

  if ((Configurator::IsRecorder() && Configurator::Get().common.recorder.enabled &&
       Configurator::Get().directx.recorder.storeAddressPinning) ||
      (Configurator::IsPlayer() &&
       Configurator::Get().directx.player.addressPinning == AddressPinningMode::STORE)) {
    addLayer(std::make_unique<AddressPinningStoreLayer>());
  } else if (Configurator::IsPlayer() &&
             Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE) {
    addLayer(std::make_unique<AddressPinningUseLayer>());
  }
}

} // namespace DirectX
} // namespace gits
