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

class PortabilityLayerGroup : public LayerGroup {
public:
  PortabilityLayerGroup() = default;
  ~PortabilityLayerGroup() override = default;

  void loadLayers() override;
};

} // namespace DirectX
} // namespace gits
