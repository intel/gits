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

class ResourceDumpingLayerGroup : public LayerGroup {
public:
  ResourceDumpingLayerGroup() = default;
  ~ResourceDumpingLayerGroup() override = default;

  void LoadLayers() override;
};

} // namespace DirectX
} // namespace gits
