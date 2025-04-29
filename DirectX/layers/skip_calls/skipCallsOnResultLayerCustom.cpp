// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "skipCallsOnResultLayerAuto.h"

namespace gits {
namespace DirectX {

void SkipCallsOnResultLayer::pre(IDXGISwapChainPresentCommand& command) {
  if (FAILED(command.result_.value)) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(IDXGISwapChain1Present1Command& command) {
  if (FAILED(command.result_.value)) {
    command.skip = true;
  }
}

} // namespace DirectX
} // namespace gits
