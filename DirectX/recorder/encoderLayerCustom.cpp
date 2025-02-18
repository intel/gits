// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "encoderLayerAuto.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"

#include "token.h"
#include "streams.h"

namespace gits {
namespace DirectX {

void EncoderLayer::post(IDXGISwapChainPresentCommand& c) {
  if (!(c.Flags_.value & DXGI_PRESENT_TEST)) {
    recorder_.frameEnd(c.key);
  }
  if (!c.skip) {
    recorder_.record(c.key, new IDXGISwapChainPresentWriter(c));
  }
}

void EncoderLayer::post(IDXGISwapChain1Present1Command& c) {
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    recorder_.frameEnd(c.key);
  }
  if (!c.skip) {
    recorder_.record(c.key, new IDXGISwapChain1Present1Writer(c));
  }
}

} // namespace DirectX
} // namespace gits
