// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureLayerManager.h"
#include "configurator.h"
#include "testLayerAuto.h"
#include "encoderLayerAuto.h"
#include "captureCustomizationLayer.h"

namespace gits {
namespace vulkan {

void CaptureLayerManager::LoadLayers(CaptureManager& captureManager, GitsRecorder& gitsRecorder) {
  auto& cfg = Configurator::Get();

  std::unique_ptr<Layer> testLayer = std::make_unique<TestLayerAuto>();
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> captureCustomizationLayer;

  if (cfg.common.recorder.enabled) {
    captureCustomizationLayer =
        std::make_unique<CaptureCustomizationLayer>(captureManager, gitsRecorder);
    encoderLayer = std::make_unique<EncoderLayer>(gitsRecorder);
  }

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  enablePreLayer(testLayer);
  enablePreLayer(captureCustomizationLayer);

  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PostLayers.push_back(layer.get());
    }
  };

  enablePostLayer(captureCustomizationLayer);
  enablePostLayer(encoderLayer);
  enablePostLayer(testLayer);

  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };

  retainLayer(std::move(captureCustomizationLayer));
  retainLayer(std::move(encoderLayer));
  retainLayer(std::move(testLayer));
}

} // namespace vulkan
} // namespace gits
