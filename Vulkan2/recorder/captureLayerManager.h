// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "tools_lite.h"
#include "orderingRecorder.h"
#include "traceLayerGroup.h"

#include <vector>
#include <memory>

namespace gits {
namespace vulkan {

class CaptureManager;
class PluginService;

class CaptureLayerManager : public gits::noncopyable {
public:
  void LoadLayers(CaptureManager& captureManager,
                  stream::OrderingRecorder& recorder,
                  PluginService& pluginService);

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
