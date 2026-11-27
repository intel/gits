#include "capabilitySuppressionLayer.h"
#include "captureManager.h"
#include "configurator.h"
#include "suppressNames.h"

namespace gits {
namespace vulkan {

void CapabilitySuppressionLayer::Pre(vkEnumerateInstanceLayerPropertiesCommand& command) {
  const auto& suppressLayers = Configurator::Get().vulkan.recorder.suppressLayers;
  if (suppressLayers.empty()) {
    return;
  }
  command.m_Skip = true;
  VkGlobalLevelDispatchTable& dispatchTable = m_Manager.GetGlobalDispatchTable();
  ProduceFilteredProperties<VkLayerProperties>(
      suppressLayers,
      [&dispatchTable](uint32_t* count, VkLayerProperties* properties) {
        return dispatchTable.vkEnumerateInstanceLayerProperties(count, properties);
      },
      [](const VkLayerProperties& layer) { return layer.layerName; },
      command.m_pPropertyCount.Value, command.m_pProperties.Value);
}

void CapabilitySuppressionLayer::Pre(vkEnumerateDeviceLayerPropertiesCommand& command) {
  const auto& suppressLayers = Configurator::Get().vulkan.recorder.suppressLayers;
  if (suppressLayers.empty()) {
    return;
  }
  command.m_Skip = true;
  VkPhysicalDevice physicalDevice = command.m_physicalDevice.Value;
  VkInstanceLevelDispatchTable& dispatchTable = m_Manager.GetInstanceDispatchTable(physicalDevice);
  ProduceFilteredProperties<VkLayerProperties>(
      suppressLayers,
      [&dispatchTable, physicalDevice](uint32_t* count, VkLayerProperties* properties) {
        return dispatchTable.vkEnumerateDeviceLayerProperties(physicalDevice, count, properties);
      },
      [](const VkLayerProperties& layer) { return layer.layerName; },
      command.m_pPropertyCount.Value, command.m_pProperties.Value);
}

void CapabilitySuppressionLayer::Pre(vkEnumerateInstanceExtensionPropertiesCommand& command) {
  const auto& suppressExtensions = Configurator::Get().vulkan.shared.suppressExtensions;
  if (suppressExtensions.empty()) {
    return;
  }
  command.m_Skip = true;
  const char* layerName = command.m_pLayerName.Value;
  VkGlobalLevelDispatchTable& dispatchTable = m_Manager.GetGlobalDispatchTable();
  ProduceFilteredProperties<VkExtensionProperties>(
      suppressExtensions,
      [&dispatchTable, layerName](uint32_t* count, VkExtensionProperties* properties) {
        return dispatchTable.vkEnumerateInstanceExtensionProperties(layerName, count, properties);
      },
      [](const VkExtensionProperties& extension) { return extension.extensionName; },
      command.m_pPropertyCount.Value, command.m_pProperties.Value);
}

void CapabilitySuppressionLayer::Pre(vkEnumerateDeviceExtensionPropertiesCommand& command) {
  const auto& suppressExtensions = Configurator::Get().vulkan.shared.suppressExtensions;
  if (suppressExtensions.empty()) {
    return;
  }
  command.m_Skip = true;
  const char* layerName = command.m_pLayerName.Value;
  VkPhysicalDevice physicalDevice = command.m_physicalDevice.Value;
  VkInstanceLevelDispatchTable& dispatchTable = m_Manager.GetInstanceDispatchTable(physicalDevice);
  ProduceFilteredProperties<VkExtensionProperties>(
      suppressExtensions,
      [&dispatchTable, physicalDevice, layerName](uint32_t* count,
                                                  VkExtensionProperties* properties) {
        return dispatchTable.vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, count,
                                                                  properties);
      },
      [](const VkExtensionProperties& extension) { return extension.extensionName; },
      command.m_pPropertyCount.Value, command.m_pProperties.Value);
}

void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeaturesCommand& command) {
  SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                 command.m_pFeatures.Value);
}

void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeatures2Command& command) {
  if (command.m_pFeatures.Value) {
    SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                   &command.m_pFeatures.Value->features);
  }
}

void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeatures2KHRCommand& command) {
  if (command.m_pFeatures.Value) {
    SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                   &command.m_pFeatures.Value->features);
  }
}

} // namespace vulkan
} // namespace gits
