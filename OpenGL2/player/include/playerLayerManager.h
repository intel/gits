// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <memory>
#include <vector>

namespace gits::opengl2 {

class Layer;
class PluginService;
class TraceLayerGroup;

class PlayerLayerManager {
public:
  // Dtor is defined in the .cpp file, as Layer is forward-declared and owned by unique_ptr.
  // Ctor is too, for symmetry.
  PlayerLayerManager();
  ~PlayerLayerManager();
  PlayerLayerManager(const PlayerLayerManager&) = delete;
  PlayerLayerManager& operator=(const PlayerLayerManager&) = delete;

  void LoadLayers();
  // Idempotent. Clears layer views, then destroys owned layers. Plugins are never unloaded:
  // Shutdown() runs from a TOPIC_PROGRAM_EXIT callback, and a plugin unsubscribing there would
  // modify the subscriber list being published. Each plugin DLL destroys its plugin when it is
  // detached at process exit.
  void Shutdown();

  // These two return non-owning views whose pointers remain valid until Shutdown():

  const std::vector<Layer*>& GetPreLayers() const {
    return m_PreLayers;
  }

  const std::vector<Layer*>& GetPostLayers() const {
    return m_PostLayers;
  }

private:
  std::unique_ptr<PluginService> m_PluginService;
  std::unique_ptr<TraceLayerGroup> m_TraceLayerGroup;
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
};

} // namespace gits::opengl2
