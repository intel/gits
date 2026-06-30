// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "layerAuto.h"
#include "traceLayerGroup.h"

#include <vector>
#include <memory>
#include <fstream>
#include <mutex>

namespace gits {
namespace vulkan {

class PlayerManager;
class PluginService;

class PlayerLayerManager : public gits::noncopyable {
public:
  void LoadLayers(PlayerManager& playerManager, PluginService& pluginService);

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }

  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
  std::unique_ptr<TraceLayerGroup> m_TraceLayerGroup;
};

} // namespace vulkan
} // namespace gits
