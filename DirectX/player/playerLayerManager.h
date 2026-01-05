// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "traceFactory.h"
#include "subcaptureFactory.h"
#include "executionSerializationFactory.h"
#include "resourceDumpingFactory.h"
#include "skipCallsFactory.h"
#include "portabilityFactory.h"
#include "addressPinningFactory.h"
#include "pluginService.h"

#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

class PlayerManager;

class PlayerLayerManager : public gits::noncopyable {
public:
  void loadLayers(PlayerManager& playerManager, PluginService& pluginService);

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

private:
  TraceFactory traceFactory_;
  SubcaptureFactory subcaptureFactory_;
  ExecutionSerializationFactory executionSerializationFactory_;
  ResourceDumpingFactory resourceDumpingFactory_;
  SkipCallsFactory skipCallsFactory_;
  PortabilityFactory portabilityFactory_;
  AddressPinningFactory addressPinningFactory_;

  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  std::vector<std::unique_ptr<Layer>> layersOwner_;
};

} // namespace DirectX
} // namespace gits
