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

  void LoadLayers(CaptureManager& captureManager,
                  stream::OrderingRecorder& gitsRecorder,
                  GpuAddressService& gpuAddressService,
                  PluginService& pluginService);

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  TraceLayerGroup m_TraceLayerGroup;
  ResourceDumpingLayerGroup m_ResourceDumpingLayerGroup;
  PortabilityLayerGroup m_PortabilityLayerGroup;
  AddressPinningLayerGroup m_AddressPinningLayerGroup;

  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
};

} // namespace DirectX
} // namespace gits
