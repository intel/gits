// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorBufferCaptureService.h"
#include "log.h"
#include "vulkanHelpers.h"

#include <limits>

namespace gits {
namespace vulkan {

namespace {

bool IsDescriptorBufferUsage(VkBufferUsageFlags2 usage) {
  return isBitSet(usage, VK_BUFFER_USAGE_2_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT) ||
         isBitSet(usage, VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT) ||
         isBitSet(usage, VK_BUFFER_USAGE_2_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT);
}

} // namespace

thread_local DescriptorBufferCaptureService::DescriptorBufferCapabilities
    DescriptorBufferCaptureService::s_DeviceCaps;
thread_local VkPhysicalDeviceBufferDeviceAddressFeatures
    DescriptorBufferCaptureService::s_BufferDeviceAddressFeatures;
thread_local VkBufferOpaqueCaptureAddressCreateInfo
    DescriptorBufferCaptureService::s_BufferOpaqueCaptureAddress;
thread_local VkMemoryOpaqueCaptureAddressAllocateInfo
    DescriptorBufferCaptureService::s_MemoryOpaqueCaptureAddress;
thread_local VkMemoryAllocateFlagsInfo DescriptorBufferCaptureService::s_MemoryAllocateFlags;
thread_local VkOpaqueCaptureDescriptorDataCreateInfoEXT
    DescriptorBufferCaptureService::s_OpaqueCreateInfo;

void DescriptorBufferCaptureService::GetPhysicalDeviceCapabilities(
    GITSKey physicalDeviceKey, VkPhysicalDevice physicalDevice) {
  {
    std::lock_guard<std::mutex> lock(m_CapsMutex);
    if (m_Caps.find(physicalDeviceKey) != m_Caps.end()) {
      return;
    }
  }

  DescriptorBufferCapabilities caps{};
  caps.Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

  const auto& dt = m_Manager.GetInstanceDispatchTable(physicalDevice);
  auto vkGetPhysicalDeviceFeatures2Unified = dt.vkGetPhysicalDeviceFeatures2
                                                 ? dt.vkGetPhysicalDeviceFeatures2
                                                 : dt.vkGetPhysicalDeviceFeatures2KHR;
  auto vkGetPhysicalDeviceProperties2Unified = dt.vkGetPhysicalDeviceProperties2
                                                   ? dt.vkGetPhysicalDeviceProperties2
                                                   : dt.vkGetPhysicalDeviceProperties2KHR;

  {
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.pNext = &vulkan12Features;

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures{};
    descriptorBufferFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
    descriptorBufferFeatures.pNext = &bufferDeviceAddressFeatures;

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &descriptorBufferFeatures;

    vkGetPhysicalDeviceFeatures2Unified(physicalDevice, &features);

    caps.DescriptorBuffer = descriptorBufferFeatures.descriptorBuffer == VK_TRUE;
    caps.DescriptorBufferCaptureReplay =
        descriptorBufferFeatures.descriptorBufferCaptureReplay == VK_TRUE;
    if (vulkan12Features.bufferDeviceAddressCaptureReplay ||
        bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay) {
      caps.BufferDeviceAddressCaptureReplay = true;
    }
  }

  {
    VkPhysicalDeviceProperties2 properties{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &caps.Properties;
    vkGetPhysicalDeviceProperties2Unified(physicalDevice, &properties);
    caps.Properties.pNext = nullptr;
  }

  std::lock_guard<std::mutex> lock(m_CapsMutex);
  m_Caps.try_emplace(physicalDeviceKey, caps);
}

DescriptorBufferCaptureService::DescriptorBufferCapabilities DescriptorBufferCaptureService::
    GetCapabilities(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_CapsMutex);
  auto it = m_Caps.find(key);
  return it != m_Caps.end() ? it->second : DescriptorBufferCapabilities{};
}

void DescriptorBufferCaptureService::OnPreCreateDevice(vkCreateDeviceCommand& command) {
  GetPhysicalDeviceCapabilities(command.m_physicalDevice.Key, command.m_physicalDevice.Value);

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  const auto physicalDeviceCaps = GetCapabilities(command.m_physicalDevice.Key);
  s_DeviceCaps = {};

  auto* pDescriptorBufferFeatures = (VkPhysicalDeviceDescriptorBufferFeaturesEXT*)getPNextStructure(
      pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT);
  if (!pDescriptorBufferFeatures || !pDescriptorBufferFeatures->descriptorBuffer) {
    return;
  }

  s_DeviceCaps.DescriptorBuffer = true;
  s_DeviceCaps.Properties = physicalDeviceCaps.Properties;

  GITS_ASSERT(physicalDeviceCaps.DescriptorBufferCaptureReplay,
              "Application enabled descriptorBuffer but physical device reports "
              "descriptorBufferCaptureReplay == VK_FALSE. GITS cannot record a correct "
              "VK_EXT_descriptor_buffer stream without capture/replay support.");
  GITS_ASSERT(
      physicalDeviceCaps.BufferDeviceAddressCaptureReplay,
      "Application enabled VK_EXT_descriptor_buffer but physical device does not support "
      "bufferDeviceAddressCaptureReplay. Descriptor buffers are bound by device address and "
      "cannot be recorded correctly without it.");

  pDescriptorBufferFeatures->descriptorBufferCaptureReplay = VK_TRUE;

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

  s_DeviceCaps.DescriptorBufferCaptureReplay = true;
}

void DescriptorBufferCaptureService::OnPostCreateDevice(vkCreateDeviceCommand& command) {
  std::lock_guard<std::mutex> lock(m_CapsMutex);
  m_Caps[command.m_pDevice.Key] = s_DeviceCaps;
}

void DescriptorBufferCaptureService::ModifyBufferCreateInfo(GITSKey deviceKey,
                                                            VkBufferCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }

  const size_t opaqueSize = caps.Properties.bufferCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.flags |= VK_BUFFER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;

  if (IsDescriptorBufferUsage(GetBufferUsage(createInfo))) {
    AddBufferUsage(createInfo, VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT);
  }
  if (isBitSet(GetBufferUsage(createInfo), VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT) &&
      caps.BufferDeviceAddressCaptureReplay) {
    createInfo.flags |= VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  }
}

void DescriptorBufferCaptureService::ModifyImageCreateInfo(GITSKey deviceKey,
                                                           VkImageCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.imageCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.flags |= VK_IMAGE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorBufferCaptureService::ModifyImageViewCreateInfo(GITSKey deviceKey,
                                                               VkImageViewCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.imageViewCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.flags |= VK_IMAGE_VIEW_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorBufferCaptureService::ModifySamplerCreateInfo(GITSKey deviceKey,
                                                             VkSamplerCreateInfo& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.samplerCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.flags |= VK_SAMPLER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorBufferCaptureService::ModifyAccelerationStructureCreateInfo(
    GITSKey deviceKey, VkAccelerationStructureCreateInfoKHR& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.accelerationStructureCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.createFlags |=
      VK_ACCELERATION_STRUCTURE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorBufferCaptureService::ModifyAccelerationStructureCreateInfo(
    GITSKey deviceKey, VkAccelerationStructureCreateInfo2KHR& createInfo) {
  const auto caps = GetCapabilities(deviceKey);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.accelerationStructureCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }
  createInfo.createFlags |=
      VK_ACCELERATION_STRUCTURE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
}

void DescriptorBufferCaptureService::InjectOpaqueCaptureDescriptorData(
    size_t dataSize,
    const void* opaqueData,
    const void*& createInfoPNext,
    uint32_t& encodedDataSize,
    std::vector<uint8_t>& encodedData) {
  if (dataSize == 0 || !opaqueData) {
    return;
  }

  GITS_ASSERT(dataSize <= std::numeric_limits<uint32_t>::max(),
              "Opaque capture descriptor data is too large to encode.");
  encodedDataSize = static_cast<uint32_t>(dataSize);
  encodedData.assign(static_cast<const uint8_t*>(opaqueData),
                     static_cast<const uint8_t*>(opaqueData) + dataSize);

  // Avoid adding a duplicate when the application supplied this node. Keep the existing node in
  // the pNext chain; the specialized coder records the driver-queried opaque data as its payload.
  const auto* pAppCreateInfo =
      static_cast<const VkOpaqueCaptureDescriptorDataCreateInfoEXT*>(getPNextStructure(
          createInfoPNext, VK_STRUCTURE_TYPE_OPAQUE_CAPTURE_DESCRIPTOR_DATA_CREATE_INFO_EXT));
  if (pAppCreateInfo) {
    LOG_INFO << "Application-provided VkOpaqueCaptureDescriptorDataCreateInfoEXT found; the "
                "existing pNext node will be retained with driver-queried opaque data.";
    return;
  }

  s_OpaqueCreateInfo = {VK_STRUCTURE_TYPE_OPAQUE_CAPTURE_DESCRIPTOR_DATA_CREATE_INFO_EXT,
                        createInfoPNext, encodedData.data()};
  createInfoPNext = &s_OpaqueCreateInfo;
}

void DescriptorBufferCaptureService::OnPostCreateBuffer(vkCreateBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.bufferCaptureReplayDescriptorDataSize;

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);

  if (opaqueSize > 0 &&
      isBitSet(pCreateInfo->flags, VK_BUFFER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    GITS_ASSERT(dt.vkGetBufferOpaqueCaptureDescriptorDataEXT,
                "descriptorBufferCaptureReplay is enabled but "
                "vkGetBufferOpaqueCaptureDescriptorDataEXT is unavailable.");
    std::vector<uint8_t> opaqueData(opaqueSize);
    VkBufferCaptureDescriptorDataInfoEXT info{
        VK_STRUCTURE_TYPE_BUFFER_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr,
        *command.m_pBuffer.Value};
    VkResult result = dt.vkGetBufferOpaqueCaptureDescriptorDataEXT(command.m_device.Value, &info,
                                                                   opaqueData.data());
    GITS_ASSERT(result == VK_SUCCESS,
                "vkGetBufferOpaqueCaptureDescriptorDataEXT failed for buffer: " +
                    std::to_string(command.m_pBuffer.Key));
    InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                      command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                      command.m_pCreateInfo.OpaqueCaptureDescriptorData);
  }

  // Pin buffer device address for replay when capture/replay was requested at create.
  // Skip if RayTracingCaptureService (or the app) already chained an opaque address.
  if (isBitSet(pCreateInfo->flags, VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT) &&
      !getPNextStructure(pCreateInfo->pNext,
                         VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO)) {
    VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr,
                                          *command.m_pBuffer.Value};
    auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                      ? dt.vkGetBufferOpaqueCaptureAddress
                                                      : dt.vkGetBufferOpaqueCaptureAddressKHR;
    GITS_ASSERT(vkGetBufferOpaqueCaptureAddressUnified,
                "Buffer device address capture/replay is enabled but the opaque capture address "
                "query is unavailable.");
    auto opaqueCaptureAddress =
        vkGetBufferOpaqueCaptureAddressUnified(command.m_device.Value, &addressInfo);
    s_BufferOpaqueCaptureAddress = {VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO,
                                    pCreateInfo->pNext, opaqueCaptureAddress};
    pCreateInfo->pNext = &s_BufferOpaqueCaptureAddress;
  }
}

void DescriptorBufferCaptureService::OnPostCreateImage(vkCreateImageCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pImage.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.imageCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->flags, VK_IMAGE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetImageOpaqueCaptureDescriptorDataEXT,
              "descriptorBufferCaptureReplay is enabled but "
              "vkGetImageOpaqueCaptureDescriptorDataEXT is unavailable.");

  std::vector<uint8_t> opaqueData(opaqueSize);
  VkImageCaptureDescriptorDataInfoEXT info = {
      VK_STRUCTURE_TYPE_IMAGE_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr, *command.m_pImage.Value};

  VkResult result =
      dt.vkGetImageOpaqueCaptureDescriptorDataEXT(command.m_device.Value, &info, opaqueData.data());
  GITS_ASSERT(result == VK_SUCCESS, "vkGetImageOpaqueCaptureDescriptorDataEXT failed for image: " +
                                        std::to_string(command.m_pImage.Key));
  InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorData);
}

void DescriptorBufferCaptureService::OnPostCreateImageView(vkCreateImageViewCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pView.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.imageViewCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->flags,
                VK_IMAGE_VIEW_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetImageViewOpaqueCaptureDescriptorDataEXT,
              "descriptorBufferCaptureReplay is enabled but "
              "vkGetImageViewOpaqueCaptureDescriptorDataEXT is unavailable.");
  std::vector<uint8_t> opaqueData(opaqueSize);
  VkImageViewCaptureDescriptorDataInfoEXT info{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr,
      *command.m_pView.Value};
  VkResult result = dt.vkGetImageViewOpaqueCaptureDescriptorDataEXT(command.m_device.Value, &info,
                                                                    opaqueData.data());
  GITS_ASSERT(result == VK_SUCCESS,
              "vkGetImageViewOpaqueCaptureDescriptorDataEXT failed for image view: " +
                  std::to_string(command.m_pView.Key));
  InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorData);
}

void DescriptorBufferCaptureService::OnPostCreateSampler(vkCreateSamplerCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pSampler.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.samplerCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->flags, VK_SAMPLER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetSamplerOpaqueCaptureDescriptorDataEXT,
              "descriptorBufferCaptureReplay is enabled but "
              "vkGetSamplerOpaqueCaptureDescriptorDataEXT is unavailable.");
  std::vector<uint8_t> opaqueData(opaqueSize);
  VkSamplerCaptureDescriptorDataInfoEXT info{
      VK_STRUCTURE_TYPE_SAMPLER_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr,
      *command.m_pSampler.Value};
  VkResult result = dt.vkGetSamplerOpaqueCaptureDescriptorDataEXT(command.m_device.Value, &info,
                                                                  opaqueData.data());
  GITS_ASSERT(result == VK_SUCCESS,
              "vkGetSamplerOpaqueCaptureDescriptorDataEXT failed for sampler: " +
                  std::to_string(command.m_pSampler.Key));
  InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorData);
}

void DescriptorBufferCaptureService::OnPostCreateAccelerationStructureKHR(
    vkCreateAccelerationStructureKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pAccelerationStructure.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.accelerationStructureCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->createFlags,
                VK_ACCELERATION_STRUCTURE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT,
              "descriptorBufferCaptureReplay is enabled but "
              "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT is unavailable.");
  std::vector<uint8_t> opaqueData(opaqueSize);
  VkAccelerationStructureCaptureDescriptorDataInfoEXT info{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr,
      *command.m_pAccelerationStructure.Value, VK_NULL_HANDLE};
  VkResult result = dt.vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
      command.m_device.Value, &info, opaqueData.data());
  GITS_ASSERT(result == VK_SUCCESS,
              "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT failed for AS: " +
                  std::to_string(command.m_pAccelerationStructure.Key));
  InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorData);
}

void DescriptorBufferCaptureService::OnPostCreateAccelerationStructure2KHR(
    vkCreateAccelerationStructure2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pAccelerationStructure.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay) {
    return;
  }
  const size_t opaqueSize = caps.Properties.accelerationStructureCaptureReplayDescriptorDataSize;
  if (opaqueSize == 0) {
    return;
  }

  auto* pCreateInfo = command.m_pCreateInfo.Value;
  if (!isBitSet(pCreateInfo->createFlags,
                VK_ACCELERATION_STRUCTURE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)) {
    return;
  }

  const auto& dt = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
  GITS_ASSERT(dt.vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT,
              "descriptorBufferCaptureReplay is enabled but "
              "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT is unavailable.");
  std::vector<uint8_t> opaqueData(opaqueSize);
  VkAccelerationStructureCaptureDescriptorDataInfoEXT info{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CAPTURE_DESCRIPTOR_DATA_INFO_EXT, nullptr,
      *command.m_pAccelerationStructure.Value, VK_NULL_HANDLE};
  VkResult result = dt.vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
      command.m_device.Value, &info, opaqueData.data());
  GITS_ASSERT(result == VK_SUCCESS,
              "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT failed for AS: " +
                  std::to_string(command.m_pAccelerationStructure.Key));
  InjectOpaqueCaptureDescriptorData(opaqueData.size(), opaqueData.data(), pCreateInfo->pNext,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorDataSize,
                                    command.m_pCreateInfo.OpaqueCaptureDescriptorData);
}

void DescriptorBufferCaptureService::OnPreAllocateMemory(vkAllocateMemoryCommand& command) {
  if (!command.m_pAllocateInfo.Value) {
    return;
  }
  const auto caps = GetCapabilities(command.m_device.Key);
  if (!caps.DescriptorBufferCaptureReplay || !caps.BufferDeviceAddressCaptureReplay) {
    return;
  }

  auto* pAllocateInfo = command.m_pAllocateInfo.Value;

  // Imports must keep opaqueCaptureAddress at zero
  // (VUID-VkMemoryAllocateInfo-opaqueCaptureAddress-03333). Unlike
  // RayTracingCaptureService, do NOT skip image-dedicated allocations:
  // VK_EXT_descriptor_buffer capture/replay requires
  // vkGetDeviceMemoryOpaqueCaptureAddress for any memory used by captured
  // buffers/images/image views/samplers/AS.
  if (IsMemoryImport(pAllocateInfo->pNext)) {
    return;
  }

  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);

  // Mesa ANV ignores VkBufferOpaqueCaptureAddressCreateInfo and derives buffer
  // addresses from the memory allocation; image descriptors similarly need the
  // backing allocation pinned. RT runs first and may already have set flags.
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

  s_MemoryAllocateFlags = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, // sType
      pAllocateInfo->pNext,                         // pNext
      flags,                                        // flags
      0                                             // deviceMask
  };
  pAllocateInfo->pNext = &s_MemoryAllocateFlags;
}

void DescriptorBufferCaptureService::OnPostAllocateMemory(vkAllocateMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pMemory.Value ||
      !command.m_pAllocateInfo.Value) {
    return;
  }
  if (!GetCapabilities(command.m_device.Key).DescriptorBufferCaptureReplay) {
    return;
  }

  auto* pAllocateInfo = command.m_pAllocateInfo.Value;
  if (IsMemoryImport(pAllocateInfo->pNext)) {
    return;
  }

  auto* pAllocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
      pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);
  if (!pAllocateFlagsInfo ||
      !isBitSet(pAllocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    return;
  }

  // RT runs first and may already have chained the opaque address.
  if (getPNextStructure(pAllocateInfo->pNext,
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
