// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"

namespace gits {
namespace DirectX {

class SkipCallsLayerGroup : public LayerGroup {
public:
  SkipCallsLayerGroup() = default;
  ~SkipCallsLayerGroup() override = default;

  void loadLayers() override;
};

} // namespace DirectX
} // namespace gits
