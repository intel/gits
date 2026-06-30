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
#include "subcaptureLayer.h"
#include "resourceDumpingLayerGroup.h"

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

  // Destroy all owned layers (and their layer groups) while the Vulkan library
  // and dispatch tables are still valid.  Must be called at playback end before
  // PlayerManager closes the driver library: the ScreenshotsLayer owns
  // SwapchainImagesDumper instances that flush their last screenshot
  // asynchronously on a worker thread, and that worker (plus the dumper's own
  // resource cleanup) needs a live device dispatch table.  Relying on member
  // destruction order would run this after the library is closed and the
  // dispatch-table maps are gone, dropping the final present's screenshot --
  // which is the ONLY screenshot for a single-frame stream.  Idempotent.
  void Shutdown();

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
  std::unique_ptr<ResourceDumpingLayerGroup> m_ResourceDumpingLayerGroup;
};

} // namespace vulkan
} // namespace gits
