// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rayTracingCaptureService.h"
#include "vulkanHelpers.h"

namespace gits {
namespace vulkan {

thread_local VkBufferOpaqueCaptureAddressCreateInfo
    RayTracingCaptureService::s_BufferOpaqueCaptureAddress;
thread_local VkMemoryOpaqueCaptureAddressAllocateInfo
    RayTracingCaptureService::s_MemoryOpaqueCaptureAddress;
thread_local RayTracingCaptureService::RayTracingCapabilities
    RayTracingCaptureService::s_DeviceCaps;

void RayTracingCaptureService::GetPhysicalDeviceCapabilities(
    HandleArgument<VkPhysicalDevice>& physicalDevice) {
  if (m_Caps.find(physicalDevice.Key) != m_Caps.end()) {
    return;
  }

  RayTracingCapabilities caps = {
      false, // bool m_BufferDeviceAddressCaptureReplay;
      false, // bool m_AccelerationStructureCaptureReplay;
      false, // bool m_RayTracingPipelineShaderGroupHandleCaptureReplay;
      0,     // uint32_t m_ShaderGroupCaptureReplayHandleSize;
      false, // bool m_Micromap;
      false  // bool m_MicromapCaptureReplay;
  };
  const auto& dt = m_Manager.GetInstanceDispatchTable(physicalDevice.Value);

  // Features
  {
    auto vkGetPhysicalDeviceFeatures2Unified = dt.vkGetPhysicalDeviceFeatures2
                                                   ? dt.vkGetPhysicalDeviceFeatures2
                                                   : dt.vkGetPhysicalDeviceFeatures2KHR;

    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.pNext = &vulkan12Features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.pNext = &bufferDeviceAddressFeatures;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
    rayTracingPipelineFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceOpacityMicromapFeaturesEXT opacityMicromapFeatures = {};
    opacityMicromapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT;
    opacityMicromapFeatures.pNext = &rayTracingPipelineFeatures;

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &opacityMicromapFeatures;

    vkGetPhysicalDeviceFeatures2Unified(physicalDevice.Value, &features);

    if (vulkan12Features.bufferDeviceAddressCaptureReplay) {
      caps.m_BufferDeviceAddressCaptureReplay = true;
    }
    if (bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay) {
      caps.m_BufferDeviceAddressCaptureReplay = true;
    }
    if (accelerationStructureFeatures.accelerationStructureCaptureReplay) {
      caps.m_AccelerationStructureCaptureReplay = true;
    }
    if (rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay) {
      caps.m_RayTracingPipelineShaderGroupHandleCaptureReplay = true;
    }
    caps.m_Micromap = opacityMicromapFeatures.micromap == VK_TRUE;
    caps.m_MicromapCaptureReplay = opacityMicromapFeatures.micromapCaptureReplay == VK_TRUE;
  }
  // Properties
  {
    auto vkGetPhysicalDeviceProperties2Unified = dt.vkGetPhysicalDeviceProperties2
                                                     ? dt.vkGetPhysicalDeviceProperties2
                                                     : dt.vkGetPhysicalDeviceProperties2KHR;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {};
    rayTracingPipelineProperties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
    physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physicalDeviceProperties.pNext = &rayTracingPipelineProperties;

    vkGetPhysicalDeviceProperties2Unified(physicalDevice.Value, &physicalDeviceProperties);

    caps.m_ShaderGroupCaptureReplayHandleSize =
        rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize;
  }

  if (!caps.m_BufferDeviceAddressCaptureReplay || !caps.m_AccelerationStructureCaptureReplay ||
      !caps.m_RayTracingPipelineShaderGroupHandleCaptureReplay) {
    std::ostringstream os;

    os << "Physical device " << physicalDevice.Key
       << " doesn't support capture/replay features for:\n";

    if (!caps.m_BufferDeviceAddressCaptureReplay) {
      os << "  - buffer device addresses\n";
    }
    if (!caps.m_AccelerationStructureCaptureReplay) {
      os << "  - acceleration structures\n";
    }
    if (!caps.m_RayTracingPipelineShaderGroupHandleCaptureReplay) {
      os << "  - ray tracing pipelines\n";
    }
    if (!caps.m_ShaderGroupCaptureReplayHandleSize) {
      os << "Shader group capture/replay handle size is 0!";
    }

    LOG_WARNING << os.str();
  }

  m_Caps[physicalDevice.Key] = caps;
}

void RayTracingCaptureService::OnPreCreateDevice(vkCreateDeviceCommand& command) {
  GetPhysicalDeviceCapabilities(command.m_physicalDevice);

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  const auto& physicalDeviceCaps = m_Caps[command.m_physicalDevice.Key];
  s_DeviceCaps = {};

  // Enable capture/replay features for buffers, acceleration structures and pipelines

  // Core 1.2
  {
    auto* pVulkan12Features = (VkPhysicalDeviceVulkan12Features*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    if (pVulkan12Features && pVulkan12Features->bufferDeviceAddress &&
        physicalDeviceCaps.m_BufferDeviceAddressCaptureReplay) {
      pVulkan12Features->bufferDeviceAddressCaptureReplay = VK_TRUE;
      s_DeviceCaps.m_BufferDeviceAddressCaptureReplay = true;
    }
  }
  // KHR
  {
    auto* pBufferDeviceAddressFeatures =
        (VkPhysicalDeviceBufferDeviceAddressFeatures*)getPNextStructure(
            pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
    if (pBufferDeviceAddressFeatures && pBufferDeviceAddressFeatures->bufferDeviceAddress &&
        physicalDeviceCaps.m_BufferDeviceAddressCaptureReplay) {
      pBufferDeviceAddressFeatures->bufferDeviceAddressCaptureReplay = VK_TRUE;
      s_DeviceCaps.m_BufferDeviceAddressCaptureReplay = true;
    }

    auto* pAccelerationStructureFeatures =
        (VkPhysicalDeviceAccelerationStructureFeaturesKHR*)getPNextStructure(
            pCreateInfo->pNext,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);
    if (pAccelerationStructureFeatures && pAccelerationStructureFeatures->accelerationStructure &&
        physicalDeviceCaps.m_AccelerationStructureCaptureReplay) {
      pAccelerationStructureFeatures->accelerationStructureCaptureReplay = VK_TRUE;
      s_DeviceCaps.m_AccelerationStructureCaptureReplay = true;
    }
  }
  // EXT
  {
    auto* pBufferDeviceAddressFeatures =
        (VkPhysicalDeviceBufferDeviceAddressFeatures*)getPNextStructure(
            pCreateInfo->pNext,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT);
    if (pBufferDeviceAddressFeatures && pBufferDeviceAddressFeatures->bufferDeviceAddress) {
      assert(0 && "GITS currently doesn't support VK_EXT_buffer_device_address extension");
    }
  }
  // Ray tracing pipeline - KHR
  {
    auto* pRayTracingPipelineFeatures =
        (VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)getPNextStructure(
            pCreateInfo->pNext,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
    if (pRayTracingPipelineFeatures && pRayTracingPipelineFeatures->rayTracingPipeline &&
        physicalDeviceCaps.m_RayTracingPipelineShaderGroupHandleCaptureReplay) {
      pRayTracingPipelineFeatures->rayTracingPipelineShaderGroupHandleCaptureReplay = VK_TRUE;
      s_DeviceCaps.m_RayTracingPipelineShaderGroupHandleCaptureReplay = true;
      s_DeviceCaps.m_ShaderGroupCaptureReplayHandleSize =
          physicalDeviceCaps.m_ShaderGroupCaptureReplayHandleSize;
    }
  }
  // Opacity micromap - EXT
  //
  // Note the asymmetry with every block above: micromapCaptureReplay is deliberately NOT
  // force-enabled, and VkMicromapCreateInfoEXT::deviceAddress is left at 0. A micromap is only
  // ever referenced by handle (dstMicromap, VkAccelerationStructureTrianglesOpacityMicromapEXT::
  // micromap, VkCopyMicromapInfoEXT), the EXT has no vkGetMicromapDeviceAddressEXT to query an
  // address with, and pinning the storage buffer's address already reproduces the placement the
  // spec's "identically created micromap" precondition asks for. See OnPostCreateMicromapEXT.
  {
    auto* pOpacityMicromapFeatures = (VkPhysicalDeviceOpacityMicromapFeaturesEXT*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT);
    if (pOpacityMicromapFeatures && pOpacityMicromapFeatures->micromap) {
      s_DeviceCaps.m_Micromap = true;
      s_DeviceCaps.m_MicromapCaptureReplay = physicalDeviceCaps.m_MicromapCaptureReplay;

      // Only warned about for apps that actually use micromaps - otherwise every non-OMM title
      // on a driver without the feature would log this.
      if (!physicalDeviceCaps.m_MicromapCaptureReplay && !m_MicromapCaptureReplayWarningIssued) {
        m_MicromapCaptureReplayWarningIssued = true;
        LOG_WARNING << "Application enabled VkPhysicalDeviceOpacityMicromapFeaturesEXT::micromap "
                       "but physical device "
                    << command.m_physicalDevice.Key
                    << " reports micromapCaptureReplay == VK_FALSE. Micromap content is restored "
                       "by rebuilding it from captured inputs, which does not need the feature, "
                       "but no address-pinning fallback is available on this driver.";
      }
    }
  }
}

void RayTracingCaptureService::OnPostCreateDevice(vkCreateDeviceCommand& command) {
  m_Caps[command.m_pDevice.Key] = s_DeviceCaps;
}

void RayTracingCaptureService::ModifyBufferCreateInfo(GITSKey deviceKey,
                                                      VkBufferCreateInfo& createInfo) {
  // MICROMAP_STORAGE is what makes a replayed micromap land on the same storage address: GITS
  // pins no micromap address of its own, so the buffer's pinned address plus the verbatim
  // createInfo.offset is the entire reproduction mechanism. MICROMAP_BUILD_INPUT_READ_ONLY is
  // deliberately not listed - build inputs are ordinary buffers, already pinned via
  // SHADER_DEVICE_ADDRESS.
  if (isBitSet(createInfo.usage, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR) ||
      isBitSet(createInfo.usage, VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT)) {
    createInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  }

  if (isBitSet(createInfo.usage, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) &&
      m_Caps[deviceKey].m_BufferDeviceAddressCaptureReplay) {
    createInfo.flags |= VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  }
}

void RayTracingCaptureService::OnPostCreateBuffer(vkCreateBufferCommand& command) {
  auto device = command.m_device.Value;
  auto* pCreateInfo = command.m_pCreateInfo.Value;

  if (isBitSet(pCreateInfo->flags, VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    VkBufferDeviceAddressInfo addressInfo = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType sType
        nullptr,                                      // const void * pNext
        *command.m_pBuffer.Value                      // VkBuffer buffer
    };

    const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                      ? dt.vkGetBufferOpaqueCaptureAddress
                                                      : dt.vkGetBufferOpaqueCaptureAddressKHR;

    auto opaqueCaptureAddress = vkGetBufferOpaqueCaptureAddressUnified(device, &addressInfo);
    s_BufferOpaqueCaptureAddress = {
        VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO, // VkStructureType sType
        pCreateInfo->pNext,                                          // const void * pNext
        opaqueCaptureAddress // uint64_t opaqueCaptureAddress
    };
    pCreateInfo->pNext = &s_BufferOpaqueCaptureAddress;
  }
}

void RayTracingCaptureService::OnPreAllocateMemory(vkAllocateMemoryCommand& command) {
  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      command.m_pAllocateInfo.Value->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);

  if (pAllocateFlagsInfo &&
      isBitSet(pAllocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT) &&
      m_Caps[command.m_device.Key].m_BufferDeviceAddressCaptureReplay) {
    pAllocateFlagsInfo->flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  }
}

void RayTracingCaptureService::OnPostAllocateMemory(vkAllocateMemoryCommand& command) {
  auto device = command.m_device.Value;
  auto* pAllocateInfo = command.m_pAllocateInfo.Value;
  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);

  if (pAllocateFlagsInfo &&
      isBitSet(pAllocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    VkDeviceMemoryOpaqueCaptureAddressInfo addressInfo = {
        VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO, // VkStructureType sType
        nullptr,                                                     // const void* pNext
        *command.m_pMemory.Value                                     // VkDeviceMemory memory
    };

    const auto& dt = m_Manager.GetDeviceDispatchTable(device);
    auto vkGetDeviceMemoryOpaqueCaptureAddressUnified =
        dt.vkGetDeviceMemoryOpaqueCaptureAddress ? dt.vkGetDeviceMemoryOpaqueCaptureAddress
                                                 : dt.vkGetDeviceMemoryOpaqueCaptureAddressKHR;

    auto opaqueCaptureAddress = vkGetDeviceMemoryOpaqueCaptureAddressUnified(device, &addressInfo);
    s_MemoryOpaqueCaptureAddress = {
        VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO, // VkStructureType sType
        pAllocateInfo->pNext,                                          // const void* pNext
        opaqueCaptureAddress // uint64_t opaqueCaptureAddress
    };
    pAllocateInfo->pNext = &s_MemoryOpaqueCaptureAddress;
  }
}

void RayTracingCaptureService::OnPostCreateAccelerationStructureKHR(
    vkCreateAccelerationStructureKHRCommand& command) {
  if (!m_Caps[command.m_device.Key].m_AccelerationStructureCaptureReplay) {
    return;
  }

  VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, // VkStructureType sType
      nullptr,                                                          // const void* pNext
      *command.m_pAccelerationStructure.Value // VkAccelerationStructureKHR accelerationStructure
  };
  auto deviceAddress =
      m_Manager.GetDeviceDispatchTable(command.m_device.Value)
          .vkGetAccelerationStructureDeviceAddressKHR(command.m_device.Value, &addressInfo);
  auto* pCreateInfo = command.m_pCreateInfo.Value;

  pCreateInfo->deviceAddress = deviceAddress;
  pCreateInfo->createFlags |=
      VK_ACCELERATION_STRUCTURE_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR;
}

void RayTracingCaptureService::OnPostCreateMicromapEXT(vkCreateMicromapEXTCommand& command) {
  // Intentionally a no-op, unlike its acceleration structure counterpart above. No micromap
  // capture/replay address is requested: nothing consumes one (every reference is by handle, and
  // the EXT has no vkGetMicromapDeviceAddressEXT to query an address with), and pinning the
  // storage buffer's address already gives the spec's own precondition for a requested address -
  // identically created micromap and buffer, same offset. Content comes from a rebuild.
}

void RayTracingCaptureService::OnPreCreateRayTracingPipelinesKHR(
    vkCreateRayTracingPipelinesKHRCommand& command) {
  command.m_deferredOperation.Value = VK_NULL_HANDLE;
  command.m_deferredOperation.Key = 0;

  if (!m_Caps[command.m_device.Key].m_RayTracingPipelineShaderGroupHandleCaptureReplay) {
    return;
  }

  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    command.m_pCreateInfos.Value[i].flags |=
        VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR;
  }
}

void RayTracingCaptureService::OnPostCreateRayTracingPipelinesKHR(
    vkCreateRayTracingPipelinesKHRCommand& command) {
  if (!m_Caps[command.m_device.Key].m_RayTracingPipelineShaderGroupHandleCaptureReplay ||
      !m_Caps[command.m_device.Key].m_ShaderGroupCaptureReplayHandleSize) {
    return;
  }

  auto device = command.m_device.Value;
  auto* pCreateInfos = command.m_pCreateInfos.Value;

  uint32_t captureReplayHandleSize =
      m_Caps[command.m_device.Key].m_ShaderGroupCaptureReplayHandleSize;
  uint32_t captureReplayHandlesDataSize = 0;

  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    captureReplayHandlesDataSize += pCreateInfos[i].groupCount;
  }
  captureReplayHandlesDataSize *= captureReplayHandleSize;

  command.m_pCreateInfos.CaptureReplayHandleSize = captureReplayHandleSize;
  command.m_pCreateInfos.CaptureReplayHandlesData.resize(captureReplayHandlesDataSize);

  uint8_t* ptr = command.m_pCreateInfos.CaptureReplayHandlesData.data();
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto groupCount = pCreateInfos[i].groupCount;

    m_Manager.GetDeviceDispatchTable(device).vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(
        device, command.m_pPipelines.Value[i], 0, groupCount, groupCount * captureReplayHandleSize,
        ptr);

    ptr += groupCount * captureReplayHandleSize;
  }
}

} // namespace vulkan
} // namespace gits
