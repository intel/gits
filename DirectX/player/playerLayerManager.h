// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "traceLayerGroup.h"
#include "subcaptureLayerGroup.h"
#include "executionSerializationLayerGroup.h"
#include "resourceDumpingLayerGroup.h"
#include "skipCallsLayerGroup.h"
#include "portabilityLayerGroup.h"
#include "addressPinningLayerGroup.h"
#include "pluginService.h"

#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

class PlayerManager;

class PlayerLayerManager {
public:
  PlayerLayerManager() = default;
  ~PlayerLayerManager() = default;
  PlayerLayerManager(const PlayerLayerManager&) = delete;
  PlayerLayerManager& operator=(const PlayerLayerManager&) = delete;

  void loadLayers(PlayerManager& playerManager, PluginService& pluginService);

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

private:
  TraceLayerGroup traceLayerGroup_;
  SubcaptureLayerGroup subcaptureLayerGroup_;
  ExecutionSerializationLayerGroup executionSerializationLayerGroup_;
  ResourceDumpingLayerGroup resourceDumpingLayerGroup_;
  SkipCallsLayerGroup skipCallsLayerGroup_;
  PortabilityLayerGroup portabilityLayerGroup_;
  AddressPinningLayerGroup addressPinningLayerGroup_;

  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  std::vector<std::unique_ptr<Layer>> layersOwner_;
};

} // namespace DirectX
} // namespace gits
