// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorHeapCaptureService.h"
#include "log.h"
#include "vulkanHelpers.h"

namespace gits {
namespace vulkan {

thread_local DescriptorHeapCaptureService::DescriptorHeapCapabilities
    DescriptorHeapCaptureService::s_DeviceCaps;
thread_local VkPhysicalDeviceBufferDeviceAddressFeatures
    DescriptorHeapCaptureService::s_BufferDeviceAddressFeatures;
thread_local VkBufferOpaqueCaptureAddressCreateInfo
    DescriptorHeapCaptureService::s_BufferOpaqueCaptureAddress;
thread_local VkMemoryOpaqueCaptureAddressAllocateInfo
    DescriptorHeapCaptureService::s_MemoryOpaqueCaptureAddress;
thread_local VkMemoryAllocateFlagsInfo DescriptorHeapCaptureService::s_MemoryAllocateFlags;
thread_local std::vector<uint8_t> DescriptorHeapCaptureService::s_OpaqueData;
thread_local VkHostAddressRangeConstEXT DescriptorHeapCaptureService::s_OpaqueDataRange;
thread_local VkOpaqueCaptureDataCreateInfoEXT DescriptorHeapCaptureService::s_OpaqueCreateInfo;

void DescriptorHeapCaptureService::GetPhysicalDeviceCapabilities(GITSKey physicalDeviceKey,
                                                                 VkPhysicalDevice physicalDevice) {
  {
    std::lock_guard<std::mutex> lock(m_CapsMutex);
    if (m_Caps.find(physicalDeviceKey) != m_Caps.end()) {
      return;
    }
  }

  DescriptorHeapCapabilities caps{};
  caps.Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;

  const auto& dt = m_Manager.GetInstanceDispatchTable(physicalDevice);
  auto vkGetPhysicalDeviceFeatures2Unified = dt.vkGetPhysicalDeviceFeatures2
                                                 ? dt.vkGetPhysicalDeviceFeatures2
                                                 : dt.vkGetPhysicalDeviceFeatures2KHR;
  auto vkGetPhysicalDeviceProperties2Unified = dt.vkGetPhysicalDeviceProperties2
                                                   ? dt.vkGetPhysicalDeviceProperties2
                                                   : dt.vkGetPhysicalDeviceProperties2KHR;

  {
    VkPhysicalDeviceVulkan12Features vulkan12Features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, &vulkan12Features};
    VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptorHeapFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
        &bufferDeviceAddressFeatures};
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                                       &descriptorHeapFeatures};

    vkGetPhysicalDeviceFeatures2Unified(physicalDevice, &features);

    caps.DescriptorHeap = descriptorHeapFeatures.descriptorHeap == VK_TRUE;
    caps.DescriptorHeapCaptureReplay =
        descriptorHeapFeatures.descriptorHeapCaptureReplay == VK_TRUE;
    caps.BufferDeviceAddressCaptureReplay =
        vulkan12Features.bufferDeviceAddressCaptureReplay == VK_TRUE ||
        bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay == VK_TRUE;
  }

  {
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                                           &caps.Properties};
    vkGetPhysicalDeviceProperties2Unified(physicalDevice, &properties);
    caps.Properties.pNext = nullptr;
  }

  std::lock_guard<std::mutex> lock(m_CapsMutex);
  m_Caps.try_emplace(physicalDeviceKey, caps);
}

DescriptorHeapCaptureService::DescriptorHeapCapabilities DescriptorHeapCaptureService::
    GetCapabilities(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_CapsMutex);
  auto it = m_Caps.find(key);
  return it != m_Caps.end() ? it->second : DescriptorHeapCapabilities{};
}

void DescriptorHeapCaptureService::OnPreCreateDevice(vkCreateDeviceCommand& command) {
  GetPhysicalDeviceCapabilities(command.m_physicalDevice.Key, command.m_physicalDevice.Value);

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  const auto physicalDeviceCaps = GetCapabilities(command.m_physicalDevice.Key);
  s_DeviceCaps = {};

  auto* pDescriptorHeapFeatures = (VkPhysicalDeviceDescriptorHeapFeaturesEXT*)getPNextStructure(
      pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT);
  if (!pDescriptorHeapFeatures || !pDescriptorHeapFeatures->descriptorHeap) {
    return;
  }

  s_DeviceCaps.DescriptorHeap = true;
  s_DeviceCaps.Properties = physicalDeviceCaps.Properties;

  GITS_ASSERT(physicalDeviceCaps.DescriptorHeapCaptureReplay,
              "Application enabled descriptorHeap but physical device reports "
              "descriptorHeapCaptureReplay == VK_FALSE. GITS cannot record a correct "
              "VK_EXT_descriptor_heap stream without capture/replay support.");
  GITS_ASSERT(
      physicalDeviceCaps.BufferDeviceAddressCaptureReplay,
      "Application enabled VK_EXT_descriptor_heap but physical device does not support "
      "bufferDeviceAddressCaptureReplay. Descriptor heap resources use device addresses and "
      "cannot be recorded correctly without it.");

  pDescriptorHeapFeatures->descriptorHeapCaptureReplay = VK_TRUE;

  {
    auto* pVulkan12Features = (VkPhysicalDeviceVulkan12Features*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    if (pVulkan12Features) {
      pVulkan12Features->bufferDeviceAddress = VK_TRUE;
      pVulkan12Features->bufferDeviceAddressCaptureReplay = VK_TRUE;
      s_DeviceCaps.BufferDeviceAddressCaptureReplay = true;
    }
  }
  {
    auto* pBufferDeviceAddressFeatures =
        (VkPhysicalDeviceBufferDeviceAddressFeatures*)getPNextStructure(
            pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
    if (pBufferDeviceAddressFeatures) {
      pBufferDeviceAddressFeatures->bufferDeviceAddress = VK_TRUE;
      pBufferDeviceAddressFeatures->bufferDeviceAddressCaptureReplay = VK_TRUE;
      s_DeviceCaps.BufferDeviceAddressCaptureReplay = true;
    }
  }
  if (!s_DeviceCaps.BufferDeviceAddressCaptureReplay) {
    s_BufferDeviceAddressFeatures = {};
    s_BufferDeviceAddressFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    s_BufferDeviceAddressFeatures.pNext = const_cast<void*>(pCreateInfo->pNext);
    s_BufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    s_BufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_TRUE;
    s_BufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
    pCreateInfo->pNext = &s_BufferDeviceAddressFeatures;
    s_DeviceCaps.BufferDeviceAddressCaptureReplay = true;
  }

  s_DeviceCaps.DescriptorHeapCaptureReplay = true;
}

void DescriptorHeapCaptureService::OnPostCreateDevice(vkCreateDeviceCommand& command) {
  std::lock_guard<std::mutex> lock(m_CapsMutex);
  m_Caps[command.m_pDevice.Key] = s_DeviceCaps;
}

void DescriptorHeapCaptureService::ModifyBufferCreateInfo(GITSKey deviceKey,
                                                          VkBufferCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorHeapCaptureReplay) {
    return;
  }

  if (isBitSet(GetBufferUsage(createInfo), VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT)) {
    AddBufferUsage(createInfo, VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT);
  }
  if (isBitSet(GetBufferUsage(createInfo), VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT) &&
      caps.BufferDeviceAddressCaptureReplay) {
    createInfo.flags |= VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  }
}

void DescriptorHeapCaptureService::ModifyImageCreateInfo(GITSKey deviceKey,
                                                         VkImageCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorHeapCaptureReplay || caps.Properties.imageCaptureReplayOpaqueDataSize == 0) {
    return;
  }
  createInfo.flags |= VK_IMAGE_CREATE_DESCRIPTOR_HEAP_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorHeapCaptureService::OnPostCreateBuffer(vkCreateBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pBuffer.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorHeapCaptureReplay ||
      !isBitSet(command.m_pCreateInfo.Value->flags,
                VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT) ||
      getPNextStructure(command.m_pCreateInfo.Value->pNext,
                        VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                    ? dt.vkGetBufferOpaqueCaptureAddress
                                                    : dt.vkGetBufferOpaqueCaptureAddressKHR;
  GITS_ASSERT(vkGetBufferOpaqueCaptureAddressUnified,
              "Buffer device address capture/replay is enabled but the opaque capture address "
              "query is unavailable.");

  VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr,
                                        *command.m_pBuffer.Value};
  auto opaqueCaptureAddress =
      vkGetBufferOpaqueCaptureAddressUnified(command.m_device.Value, &addressInfo);
  s_BufferOpaqueCaptureAddress = {VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO,
                                  command.m_pCreateInfo.Value->pNext, opaqueCaptureAddress};
  command.m_pCreateInfo.Value->pNext = &s_BufferOpaqueCaptureAddress;
}

void DescriptorHeapCaptureService::OnPostCreateImage(vkCreateImageCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pImage.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorHeapCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.imageCaptureReplayOpaqueDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->flags, VK_IMAGE_CREATE_DESCRIPTOR_HEAP_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  if (getPNextStructure(pCreateInfo->pNext,
                        VK_STRUCTURE_TYPE_OPAQUE_CAPTURE_DATA_CREATE_INFO_EXT)) {
    LOG_INFO << "Application-provided VkOpaqueCaptureDataCreateInfoEXT found; the existing pNext "
                "node and its opaque image data will be retained.";
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetImageOpaqueCaptureDataEXT,
              "descriptorHeapCaptureReplay is enabled but vkGetImageOpaqueCaptureDataEXT is "
              "unavailable.");

  s_OpaqueData.resize(opaqueSize);
  VkHostAddressRangeEXT captureData{s_OpaqueData.data(), opaqueSize};
  VkResult result = dt.vkGetImageOpaqueCaptureDataEXT(command.m_device.Value, 1,
                                                      command.m_pImage.Value, &captureData);
  GITS_ASSERT(result == VK_SUCCESS, "vkGetImageOpaqueCaptureDataEXT failed for image: " +
                                        std::to_string(command.m_pImage.Key));

  s_OpaqueDataRange = {s_OpaqueData.data(), opaqueSize};
  s_OpaqueCreateInfo = {VK_STRUCTURE_TYPE_OPAQUE_CAPTURE_DATA_CREATE_INFO_EXT, pCreateInfo->pNext,
                        &s_OpaqueDataRange};
  pCreateInfo->pNext = &s_OpaqueCreateInfo;
}

void DescriptorHeapCaptureService::OnPostRegisterCustomBorderColor(
    vkRegisterCustomBorderColorEXTCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pIndex.Value &&
      GetCapabilities(command.m_device.Key).DescriptorHeapCaptureReplay) {
    // The capture call already completed with the application's requestIndex value. Record true so
    // replay asks the implementation for the captured index written to pIndex.
    command.m_requestIndex.Value = VK_TRUE;
  }
}

void DescriptorHeapCaptureService::OnPreAllocateMemory(vkAllocateMemoryCommand& command) {
  if (!command.m_pAllocateInfo.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorHeapCaptureReplay || !caps.BufferDeviceAddressCaptureReplay) {
    return;
  }

  auto* pAllocateInfo = command.m_pAllocateInfo.Value;
  if (IsMemoryImport(pAllocateInfo->pNext)) {
    return;
  }

  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);
  if (pAllocateFlagsInfo &&
      isBitSet(pAllocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)) {
    pAllocateFlagsInfo->flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
    return;
  }

  const VkMemoryAllocateFlags flags =
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  if (pAllocateFlagsInfo) {
    pAllocateFlagsInfo->flags |= flags;
    return;
  }

  s_MemoryAllocateFlags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, pAllocateInfo->pNext,
                           flags, 0};
  pAllocateInfo->pNext = &s_MemoryAllocateFlags;
}

void DescriptorHeapCaptureService::OnPostAllocateMemory(vkAllocateMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pMemory.Value ||
      !command.m_pAllocateInfo.Value ||
      !GetCapabilities(command.m_device.Key).DescriptorHeapCaptureReplay) {
    return;
  }

  auto* pAllocateInfo = command.m_pAllocateInfo.Value;
  if (IsMemoryImport(pAllocateInfo->pNext)) {
    return;
  }

  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);
  if (!pAllocateFlagsInfo ||
      !isBitSet(pAllocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT) ||
      getPNextStructure(pAllocateInfo->pNext,
                        VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  auto vkGetDeviceMemoryOpaqueCaptureAddressUnified =
      dt.vkGetDeviceMemoryOpaqueCaptureAddress ? dt.vkGetDeviceMemoryOpaqueCaptureAddress
                                               : dt.vkGetDeviceMemoryOpaqueCaptureAddressKHR;
  GITS_ASSERT(vkGetDeviceMemoryOpaqueCaptureAddressUnified,
              "Memory device address capture/replay is enabled but the opaque capture address "
              "query is unavailable.");

  VkDeviceMemoryOpaqueCaptureAddressInfo addressInfo{
      VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO, nullptr,
      *command.m_pMemory.Value};
  auto opaqueCaptureAddress =
      vkGetDeviceMemoryOpaqueCaptureAddressUnified(command.m_device.Value, &addressInfo);
  s_MemoryOpaqueCaptureAddress = {VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO,
                                  pAllocateInfo->pNext, opaqueCaptureAddress};
  pAllocateInfo->pNext = &s_MemoryOpaqueCaptureAddress;
}

} // namespace vulkan
} // namespace gits
