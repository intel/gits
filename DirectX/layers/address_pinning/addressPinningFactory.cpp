// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningFactory.h"
#include "configurationLib.h"

#include "addressPinningUseLayer.h"
#include "addressPinningStoreLayer.h"

namespace gits {
namespace DirectX {

AddressPinningFactory::AddressPinningFactory() {

  if ((Configurator::IsRecorder() && Configurator::Get().common.recorder.enabled &&
       Configurator::Get().directx.capture.storeAddressPinning) ||
      (Configurator::IsPlayer() &&
       Configurator::Get().directx.player.addressPinning == AddressPinningMode::STORE)) {
    addressPinningLayer_ = std::make_unique<AddressPinningStoreLayer>();
  } else if (Configurator::IsPlayer() &&
             Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE) {
    addressPinningLayer_ = std::make_unique<AddressPinningUseLayer>();
  } else {
    addressPinningLayer_ = nullptr;
  }
}

std::unique_ptr<Layer> AddressPinningFactory::getAddressPinningLayer() {
  return std::move(addressPinningLayer_);
}

} // namespace DirectX
} // namespace gits
