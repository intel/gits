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
      0      // uint32_t m_ShaderGroupCaptureReplayHandleSize;
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

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &rayTracingPipelineFeatures;

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
}

void RayTracingCaptureService::OnPostCreateDevice(vkCreateDeviceCommand& command) {
  m_Caps[command.m_pDevice.Key] = s_DeviceCaps;
}

void RayTracingCaptureService::OnPreCreateBuffer(vkCreateBufferCommand& command) {
  auto* pCreateInfo = command.m_pCreateInfo.Value;

  if (isBitSet(pCreateInfo->usage, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)) {
    pCreateInfo->usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  }

  if (isBitSet(pCreateInfo->usage, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) &&
      m_Caps[command.m_device.Key].m_BufferDeviceAddressCaptureReplay) {
    pCreateInfo->flags |= VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
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
  // Empty - currently device address of a micromap is not used anywhere in the Vulkan API.
  // What's more, there is actually no way to acquire it (except for calculating it manually
  // based on a device address of a buffer in which micromap is stored).
  TODO("Update this function when micromap's device address starts being actually used anywhere!");
}

void RayTracingCaptureService::OnPreCreateRayTracingPipelinesKHR(
    vkCreateRayTracingPipelinesKHRCommand& command) {
  command.m_deferredOperation.Value = VK_NULL_HANDLE;

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
