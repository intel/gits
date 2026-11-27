// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayerGroup.h"
#include "captureCustomizationLayer.h"
#include "memoryRequirementsCaptureLayer.h"
#include "configurator.h"

namespace gits {
namespace vulkan {

void CaptureCustomizationLayerGroup::loadLayers() {
  auto customizationLayer = std::make_unique<CaptureCustomizationLayer>(m_Manager, m_Recorder);
  m_PreLayers.push_back(customizationLayer.get());
  m_PostLayers.push_back(customizationLayer.get());
  addLayer(std::move(customizationLayer));

  const auto& config = Configurator::Get().vulkan.recorder;
  const auto& sizeIncrease = config.portability.imageMemoryRequirementIncrease.size;
  const auto alignment = config.portability.imageMemoryRequirementIncrease.alignment;

  if (sizeIncrease.fixedAmount != 0 || sizeIncrease.percent != 0 || alignment != 0) {
    auto memoryRequirementsLayer = std::make_unique<MemoryRequirementsCaptureLayer>(
        sizeIncrease.fixedAmount, sizeIncrease.percent, alignment);
    m_PreLayers.push_back(memoryRequirementsLayer.get());
    m_PostLayers.push_back(memoryRequirementsLayer.get());
    addLayer(std::move(memoryRequirementsLayer));
  }
}

} // namespace vulkan
} // namespace gits
