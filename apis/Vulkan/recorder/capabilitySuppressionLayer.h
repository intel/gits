#pragma once

#include "layerAuto.h"

namespace gits {
namespace vulkan {

class CaptureManager;

class CapabilitySuppressionLayer : public Layer {
public:
  explicit CapabilitySuppressionLayer(CaptureManager& manager)
      : Layer("CapabilitySuppression"), m_Manager(manager) {}

  void Pre(vkEnumerateInstanceLayerPropertiesCommand& command) override;
  void Pre(vkEnumerateDeviceLayerPropertiesCommand& command) override;
  void Pre(vkEnumerateInstanceExtensionPropertiesCommand& command) override;
  void Pre(vkEnumerateDeviceExtensionPropertiesCommand& command) override;

  void Post(vkGetPhysicalDeviceFeaturesCommand& command) override;
  void Post(vkGetPhysicalDeviceFeatures2Command& command) override;
  void Post(vkGetPhysicalDeviceFeatures2KHRCommand& command) override;

private:
  CaptureManager& m_Manager;
};

} // namespace vulkan
} // namespace gits
