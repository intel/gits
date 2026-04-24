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

  void LoadLayers(PlayerManager& playerManager, PluginService& pluginService);

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  TraceLayerGroup m_TraceLayerGroup;
  SubcaptureLayerGroup m_SubcaptureLayerGroup;
  ExecutionSerializationLayerGroup m_ExecutionSerializationLayerGroup;
  ResourceDumpingLayerGroup m_ResourceDumpingLayerGroup;
  SkipCallsLayerGroup m_SkipCallsLayerGroup;
  PortabilityLayerGroup m_PortabilityLayerGroup;
  AddressPinningLayerGroup m_AddressPinningLayerGroup;

  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
};

} // namespace DirectX
} // namespace gits
