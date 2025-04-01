// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(D3D12CreateDeviceCommand& command) override;
  void post(D3D12CreateDeviceCommand& command) override;
};

} // namespace DirectX
} // namespace gits
