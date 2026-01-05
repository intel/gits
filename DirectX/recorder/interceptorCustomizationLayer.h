// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "iunknownWrapper.h"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace gits {
namespace DirectX {

class InterceptorCustomizationLayer : public Layer {
public:
  InterceptorCustomizationLayer() : Layer("InterceptorCustomization") {}

  void post(IDXGISwapChainGetBufferCommand& command) override;
  void pre(IUnknownReleaseCommand& command) override;

  void pre(D3D12CreateDeviceCommand& command) override;
  void post(D3D12CreateDeviceCommand& command) override;

private:
  std::unordered_map<unsigned, unsigned> swapChainByBufferKey_;
  std::unordered_map<unsigned, std::unordered_set<IUnknown*>> buffersBySwapChainKey_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
