// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace DirectX {

/*
 * Encapsulates creation logic of a Layer for skipping API calls.
 */
class SkipCallsFactory {
public:
  SkipCallsFactory();
  std::unique_ptr<Layer> getSkipCallsLayer();

private:
  std::unique_ptr<Layer> skipCallsLayer_;
};

} // namespace DirectX
} // namespace gits
