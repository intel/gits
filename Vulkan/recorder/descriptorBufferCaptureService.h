// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "captureManager.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

namespace gits {
namespace vulkan {

class DescriptorBufferCaptureService {
public:
  DescriptorBufferCaptureService(CaptureManager& manager) : m_Manager(manager) {}

  void OnPreCreateDevice(vkCreateDeviceCommand& command);
  void OnPostCreateDevice(vkCreateDeviceCommand& command);

  void ModifyBufferCreateInfo(GITSKey deviceKey, VkBufferCreateInfo& createInfo);
  void ModifyImageCreateInfo(GITSKey deviceKey, VkImageCreateInfo& createInfo);
  void ModifyImageViewCreateInfo(GITSKey deviceKey, VkImageViewCreateInfo& createInfo);
  void ModifySamplerCreateInfo(GITSKey deviceKey, VkSamplerCreateInfo& createInfo);
  void ModifyAccelerationStructureCreateInfo(GITSKey deviceKey,
                                             VkAccelerationStructureCreateInfoKHR& createInfo);
  void ModifyAccelerationStructureCreateInfo(GITSKey deviceKey,
                                             VkAccelerationStructureCreateInfo2KHR& createInfo);

  void OnPostCreateBuffer(vkCreateBufferCommand& command);
  void OnPostCreateImage(vkCreateImageCommand& command);
  void OnPostCreateImageView(vkCreateImageViewCommand& command);
  void OnPostCreateSampler(vkCreateSamplerCommand& command);
  void OnPostCreateAccelerationStructureKHR(vkCreateAccelerationStructureKHRCommand& command);
  void OnPostCreateAccelerationStructure2KHR(vkCreateAccelerationStructure2KHRCommand& command);

  void OnPreAllocateMemory(vkAllocateMemoryCommand& command);
  void OnPostAllocateMemory(vkAllocateMemoryCommand& command);

private:
  struct DescriptorBufferCapabilities {
    bool DescriptorBuffer{false};
    // Enables *_DESCRIPTOR_BUFFER_CAPTURE_REPLAY create flags (and related BDA create flags)
    // and Get*OpaqueCaptureDescriptorDataEXT + pNext injection.
    bool DescriptorBufferCaptureReplay{false};
    bool BufferDeviceAddressCaptureReplay{false};
    VkPhysicalDeviceDescriptorBufferPropertiesEXT Properties{};
  };

  void GetPhysicalDeviceCapabilities(GITSKey physicalDeviceKey, VkPhysicalDevice physicalDevice);
  DescriptorBufferCapabilities GetCapabilities(GITSKey key);

  void InjectOpaqueCaptureDescriptorData(size_t dataSize,
                                         const void* opaqueData,
                                         const void*& createInfoPNext,
                                         uint32_t& encodedDataSize,
                                         std::vector<uint8_t>& encodedData);

  CaptureManager& m_Manager;
  // Written on vkCreateDevice, read from create/allocate and memory-requirement queries that the
  // wrappers do not serialize against device creation.
  std::mutex m_CapsMutex;
  std::map<GITSKey, DescriptorBufferCapabilities> m_Caps;
  static thread_local DescriptorBufferCapabilities s_DeviceCaps;
  static thread_local VkPhysicalDeviceBufferDeviceAddressFeatures s_BufferDeviceAddressFeatures;
  static thread_local VkBufferOpaqueCaptureAddressCreateInfo s_BufferOpaqueCaptureAddress;
  static thread_local VkMemoryOpaqueCaptureAddressAllocateInfo s_MemoryOpaqueCaptureAddress;
  static thread_local VkMemoryAllocateFlagsInfo s_MemoryAllocateFlags;
  // Scratch node valid until EncoderLayer serializes the command.
  static thread_local VkOpaqueCaptureDescriptorDataCreateInfoEXT s_OpaqueCreateInfo;
};

} // namespace vulkan
} // namespace gits
