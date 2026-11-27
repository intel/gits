// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capabilitySuppressionLayer.h"
#include "captureManager.h"
#include "configurator.h"
#include "log.h"
#include "suppressNames.h"

namespace gits {
namespace vulkan {

CapabilitySuppressionLayer::CapabilitySuppressionLayer(CaptureManager& manager)
    : Layer("CapabilitySuppression"), m_Manager(manager) {
  for (const auto& selector : Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures) {
    if (!IsKnownPhysicalDeviceFeature(selector)) {
      LOG_WARNING << "Unknown or unsupported physical device feature suppression selector: "
                  << selector;
    }
  }
}

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

void CapabilitySuppressionLayer::Pre(vkCreateDeviceCommand& command) {
  const auto& suppressedFeatures = Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures;
  const VkDeviceCreateInfo* createInfo = command.m_pCreateInfo.Value;
  if (suppressedFeatures.empty() || createInfo == nullptr) {
    return;
  }

  auto warnIfSuppressed = [&](const char* structureName, const char* featureName,
                              const VkBool32& value) {
    if (value != VK_FALSE &&
        std::any_of(suppressedFeatures.begin(), suppressedFeatures.end(),
                    [&](const std::string& selector) {
                      return MatchesPhysicalDeviceFeature(selector, structureName, featureName);
                    })) {
      LOG_WARNING << "Application enables physical device feature " << structureName << "."
                  << featureName
                  << " configured for suppression during capture; the device-create request is "
                     "unchanged, but replay may be invalid.";
    }
  };

  if (createInfo->pEnabledFeatures != nullptr) {
    VisitPhysicalDeviceFeatures(*createInfo->pEnabledFeatures, warnIfSuppressed);
  }
  VisitPhysicalDeviceFeatureChain(createInfo->pNext, warnIfSuppressed);
}

// Filter the query results after the driver fills them, before returning them to the application.
void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeaturesCommand& command) {
  SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                 command.m_pFeatures.Value);
}

void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeatures2Command& command) {
  SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                 command.m_pFeatures.Value);
}

void CapabilitySuppressionLayer::Post(vkGetPhysicalDeviceFeatures2KHRCommand& command) {
  SuppressPhysicalDeviceFeatures(Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
                                 command.m_pFeatures.Value);
}

} // namespace vulkan
} // namespace gits
