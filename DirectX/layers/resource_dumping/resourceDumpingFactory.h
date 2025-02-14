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
 * Encapsulates creation logic of Layers related to resource dumping.
 * Validates relevant config and decides which layers to create.
 */
class ResourceDumpingFactory {
public:
  ResourceDumpingFactory();

  std::unique_ptr<Layer> getScreenshotsLayer() {
    return std::move(screenshotsLayer_);
  }
  std::unique_ptr<Layer> getResourceDumpLayer() {
    return std::move(resourceDumpLayer_);
  }
  std::unique_ptr<Layer> getRenderTargetsDumpLayer() {
    return std::move(renderTargetsDumpLayer_);
  }
  std::unique_ptr<Layer> getAccelerationStructuresDumpLayer() {
    return std::move(accelerationStructuresDumpLayer_);
  }

private:
  std::unique_ptr<Layer> screenshotsLayer_;
  std::unique_ptr<Layer> resourceDumpLayer_;
  std::unique_ptr<Layer> renderTargetsDumpLayer_;
  std::unique_ptr<Layer> accelerationStructuresDumpLayer_;
};

} // namespace DirectX
} // namespace gits
