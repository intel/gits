// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace gits {
namespace vulkan {

// Removes every entry listed in `suppressed` from a Vulkan create-info name
// array (e.g. ppEnabledLayerNames / ppEnabledExtensionNames). The surviving
// pointers are copied into `storage`, and `count`/`names` are updated to
// reference it. `storage` must outlive the API call that consumes `names`.
//
// The original array is left untouched (it may be const stream/app memory), so
// this is a no-op unless something is actually suppressed. Returns the number
// of removed entries.
inline uint32_t RemoveSuppressedNames(const std::vector<std::string>& suppressed,
                                      uint32_t& count,
                                      const char* const*& names,
                                      std::vector<const char*>& storage) {
  if (suppressed.empty() || count == 0 || names == nullptr) {
    return 0;
  }

  storage.clear();
  storage.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const char* name = names[i];
    const bool drop =
        name != nullptr && std::any_of(suppressed.begin(), suppressed.end(),
                                       [name](const std::string& s) { return s == name; });
    if (!drop) {
      storage.push_back(name);
    }
  }

  const uint32_t removed = count - static_cast<uint32_t>(storage.size());
  if (removed > 0) {
    count = static_cast<uint32_t>(storage.size());
    names = storage.data();
  }
  return removed;
}

// Enumerates a property list via `driverEnumerate` (a callable with the same
// signature as vkEnumerate{Instance,Device}{Layer,Extension}Properties, minus
// any handle the caller should already have bound), drops every entry whose
// name (obtained through `nameOf`) is listed in `suppressed` and writes the
// filtered result into `pProperties`, honoring the Vulkan two-call enumeration
// idiom (pProperties == nullptr queries the count).
template <typename Property, typename DriverEnumerate, typename NameOf>
void ProduceFilteredProperties(const std::vector<std::string>& suppressed,
                               DriverEnumerate&& driverEnumerate,
                               NameOf&& nameOf,
                               uint32_t* pPropertyCount,
                               Property* pProperties) {
  if (pPropertyCount == nullptr) {
    return;
  }

  uint32_t count = 0;
  std::vector<Property> properties;
  if (driverEnumerate(&count, nullptr) == VK_SUCCESS && count > 0) {
    properties.resize(count);
    if (driverEnumerate(&count, properties.data()) == VK_SUCCESS) {
      properties.resize(count);
      properties.erase(std::remove_if(properties.begin(), properties.end(),
                                      [&](const Property& property) {
                                        const char* name = nameOf(property);
                                        return std::any_of(
                                            suppressed.begin(), suppressed.end(),
                                            [name](const std::string& s) { return s == name; });
                                      }),
                       properties.end());
    }
  }

  if (pProperties == nullptr) {
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  } else {
    const uint32_t writable = std::min(*pPropertyCount, static_cast<uint32_t>(properties.size()));
    for (uint32_t i = 0; i < writable; ++i) {
      pProperties[i] = properties[i];
    }
    *pPropertyCount = writable;
  }
}

// Clears every VkPhysicalDeviceFeatures field whose identifier is listed in
// `suppressed`, matching the member names from the VkPhysicalDeviceFeatures
// struct definition (and the Common.Vulkan.Shared.SuppressPhysicalDeviceFeatures
// option). Returns the number of features that were suppressed.
inline uint32_t SuppressPhysicalDeviceFeatures(const std::vector<std::string>& suppressed,
                                               VkPhysicalDeviceFeatures* features) {
  if (features == nullptr || suppressed.empty()) {
    return 0;
  }

  uint32_t suppressedCount = 0;
  for (const auto& name : suppressed) {
#define GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(feature)                                             \
  if (name == #feature) {                                                                          \
    features->feature = VK_FALSE;                                                                  \
    ++suppressedCount;                                                                             \
    continue;                                                                                      \
  }

    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(robustBufferAccess)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(fullDrawIndexUint32)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(imageCubeArray)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(independentBlend)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(geometryShader)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(tessellationShader)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sampleRateShading)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(dualSrcBlend)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(logicOp)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(multiDrawIndirect)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(drawIndirectFirstInstance)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(depthClamp)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(depthBiasClamp)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(fillModeNonSolid)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(depthBounds)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(wideLines)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(largePoints)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(alphaToOne)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(multiViewport)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(samplerAnisotropy)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(textureCompressionETC2)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(textureCompressionASTC_LDR)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(textureCompressionBC)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(occlusionQueryPrecise)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(pipelineStatisticsQuery)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(vertexPipelineStoresAndAtomics)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(fragmentStoresAndAtomics)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderTessellationAndGeometryPointSize)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderImageGatherExtended)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageImageExtendedFormats)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageImageMultisample)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderClipDistance)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderCullDistance)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderFloat64)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderInt64)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderInt16)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderResourceResidency)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(shaderResourceMinLod)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseBinding)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidencyBuffer)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage2D)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage3D)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidency2Samples)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidency4Samples)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidency8Samples)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidency16Samples)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(sparseResidencyAliased)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(variableMultisampleRate)
    GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE(inheritedQueries)

#undef GITS_SUPPRESS_PHYSICAL_DEVICE_FEATURE
  }
  return suppressedCount;
}

} // namespace vulkan
} // namespace gits
