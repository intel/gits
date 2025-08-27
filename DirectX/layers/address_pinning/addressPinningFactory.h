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

class AddressPinningFactory {
public:
  AddressPinningFactory();
  std::unique_ptr<Layer> getAddressPinningLayer();

private:
  std::unique_ptr<Layer> addressPinningLayer_;
};

} // namespace DirectX
} // namespace gits
