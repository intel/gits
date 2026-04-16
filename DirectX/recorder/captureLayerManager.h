// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "pluginService.h"
#include "traceLayerGroup.h"
#include "resourceDumpingLayerGroup.h"
#include "portabilityLayerGroup.h"
#include "addressPinningLayerGroup.h"
#include "orderingRecorder.h"
#include "gpuAddressService.h"

#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

class CaptureManager;

class CaptureLayerManager {
public:
  CaptureLayerManager() = default;
  ~CaptureLayerManager() = default;
  CaptureLayerManager(const CaptureLayerManager&) = delete;
  CaptureLayerManager& operator=(const CaptureLayerManager&) = delete;

  void loadLayers(CaptureManager& captureManager,
                  stream::OrderingRecorder& gitsRecorder,
                  GpuAddressService& gpuAddressService,
                  PluginService& pluginService);

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

private:
  TraceLayerGroup traceLayerGroup_;
  ResourceDumpingLayerGroup resourceDumpingLayerGroup_;
  PortabilityLayerGroup portabilityLayerGroup_;
  AddressPinningLayerGroup addressPinningLayerGroup_;

  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  std::vector<std::unique_ptr<Layer>> layersOwner_;
};

} // namespace DirectX
} // namespace gits
