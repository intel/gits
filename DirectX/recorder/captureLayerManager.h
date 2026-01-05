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
#include "traceFactory.h"
#include "resourceDumpingFactory.h"
#include "portabilityFactory.h"
#include "addressPinningFactory.h"
#include "gitsRecorder.h"
#include "gpuAddressService.h"

#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

class CaptureManager;

class CaptureLayerManager : public gits::noncopyable {
public:
  void loadLayers(CaptureManager& captureManager,
                  GitsRecorder& gitsRecorder,
                  GpuAddressService& gpuAddressService,
                  PluginService& pluginService);

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

private:
  TraceFactory traceFactory_;
  ResourceDumpingFactory resourceDumpingFactory_;
  PortabilityFactory portabilityFactory_;
  AddressPinningFactory addressPinningFactory_;

  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  std::vector<std::unique_ptr<Layer>> layersOwner_;
};

} // namespace DirectX
} // namespace gits
