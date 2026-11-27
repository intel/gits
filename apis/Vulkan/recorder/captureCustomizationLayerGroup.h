// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"
#include "orderingRecorder.h"

#include <vector>

namespace gits {
namespace vulkan {

class CaptureManager;

class CaptureCustomizationLayerGroup : public LayerGroup {
public:
  CaptureCustomizationLayerGroup(CaptureManager& manager, stream::OrderingRecorder& recorder)
      : m_Manager(manager), m_Recorder(recorder) {
    loadLayers();
  }

  const std::vector<Layer*>& GetPreLayers() const {
    return m_PreLayers;
  }

  const std::vector<Layer*>& GetPostLayers() const {
    return m_PostLayers;
  }

private:
  void loadLayers() override;

  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
};

} // namespace vulkan
} // namespace gits
