// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "skipCallsFactory.h"
#include "config.h"
#include "configurationLib.h"
#include "skipCallsOnConfigLayerAuto.h"
#include "skipCallsOnResultLayerAuto.h"

namespace gits {
namespace DirectX {

SkipCallsFactory::SkipCallsFactory() {
  if (Configurator::Get().directx.features.skipCalls.enabled) {
    skipCallsOnConfigLayer_ = std::make_unique<SkipCallsOnConfigLayer>();
  }
  skipCallsOnResultLayer_ = std::make_unique<SkipCallsOnResultLayer>();
}

std::unique_ptr<Layer> SkipCallsFactory::getSkipCallsOnConfigLayer() {
  return std::move(skipCallsOnConfigLayer_);
}

std::unique_ptr<Layer> SkipCallsFactory::getSkipCallsOnResultLayer() {
  return std::move(skipCallsOnResultLayer_);
}

} // namespace DirectX
} // namespace gits
