// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace vulkan {

class CaptureManager;

class CapabilitySuppressionLayer : public Layer {
public:
  explicit CapabilitySuppressionLayer(CaptureManager& manager);

  void Pre(vkEnumerateInstanceLayerPropertiesCommand& command) override;
  void Pre(vkEnumerateDeviceLayerPropertiesCommand& command) override;
  void Pre(vkEnumerateInstanceExtensionPropertiesCommand& command) override;
  void Pre(vkEnumerateDeviceExtensionPropertiesCommand& command) override;
  void Pre(vkCreateDeviceCommand& command) override;

  void Post(vkGetPhysicalDeviceFeaturesCommand& command) override;
  void Post(vkGetPhysicalDeviceFeatures2Command& command) override;
  void Post(vkGetPhysicalDeviceFeatures2KHRCommand& command) override;

private:
  CaptureManager& m_Manager;
};

} // namespace vulkan
} // namespace gits
