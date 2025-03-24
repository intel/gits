// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <memory>

namespace gits {
namespace DirectX {

class PortabilityFactory {
public:
  PortabilityFactory();

  std::unique_ptr<Layer> getPortabilityLayer() {
    return std::move(portabilityLayer_);
  }

private:
  std::unique_ptr<Layer> portabilityLayer_;
};

} // namespace DirectX
} // namespace gits
