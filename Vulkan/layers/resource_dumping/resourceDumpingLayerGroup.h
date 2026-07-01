// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"
#include "screenshotsLayer.h"

namespace gits {
namespace vulkan {

class DispatchTablesHolder;

class ResourceDumpingLayerGroup : public LayerGroup {
public:
  ResourceDumpingLayerGroup() = delete;
  ResourceDumpingLayerGroup(DispatchTablesHolder& dispatchTablesHolder);
  ~ResourceDumpingLayerGroup() override = default;

  void loadLayers() override;

  ScreenshotsLayer* getScreenshotLayer() {
    return static_cast<ScreenshotsLayer*>(getLayer(ScreenshotsLayer::LAYER_NAME));
  }

private:
  DispatchTablesHolder& m_DispatchTablesHolder;
};

} // namespace vulkan
} // namespace gits
