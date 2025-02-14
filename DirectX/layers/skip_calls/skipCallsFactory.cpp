// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "skipCallsFactory.h"
#include "config.h"
#include "skipCallsLayerAuto.h"

namespace gits {
namespace DirectX {

SkipCallsFactory::SkipCallsFactory() {
  if (Config::Get().directx.features.skipCalls.enabled) {
    skipCallsLayer_ = std::make_unique<SkipCallsLayer>();
  }
}

std::unique_ptr<Layer> SkipCallsFactory::getSkipCallsLayer() {
  return std::move(skipCallsLayer_);
}

} // namespace DirectX
} // namespace gits
