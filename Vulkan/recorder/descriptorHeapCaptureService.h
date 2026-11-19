// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "captureManager.h"
#include "commandsAuto.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

namespace gits {
namespace vulkan {

class DescriptorHeapCaptureService {
public:
  DescriptorHeapCaptureService(CaptureManager& manager) : m_Manager(manager) {}

  void OnPreCreateDevice(vkCreateDeviceCommand& command);
  void OnPostCreateDevice(vkCreateDeviceCommand& command);

  void ModifyBufferCreateInfo(GITSKey deviceKey, VkBufferCreateInfo& createInfo);
  void ModifyImageCreateInfo(GITSKey deviceKey, VkImageCreateInfo& createInfo);

  void OnPostCreateBuffer(vkCreateBufferCommand& command);
  void OnPostCreateImage(vkCreateImageCommand& command);
  void OnPostRegisterCustomBorderColor(vkRegisterCustomBorderColorEXTCommand& command);

  void OnPreAllocateMemory(vkAllocateMemoryCommand& command);
  void OnPostAllocateMemory(vkAllocateMemoryCommand& command);

private:
  struct DescriptorHeapCapabilities {
    bool DescriptorHeap{false};
    bool DescriptorHeapCaptureReplay{false};
    bool BufferDeviceAddressCaptureReplay{false};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT Properties{};
  };

  void GetPhysicalDeviceCapabilities(GITSKey physicalDeviceKey, VkPhysicalDevice physicalDevice);
  DescriptorHeapCapabilities GetCapabilities(GITSKey key);

  CaptureManager& m_Manager;
  std::mutex m_CapsMutex;
  std::map<GITSKey, DescriptorHeapCapabilities> m_Caps;
  static thread_local DescriptorHeapCapabilities s_DeviceCaps;
  static thread_local VkPhysicalDeviceBufferDeviceAddressFeatures s_BufferDeviceAddressFeatures;
  static thread_local VkBufferOpaqueCaptureAddressCreateInfo s_BufferOpaqueCaptureAddress;
  static thread_local VkMemoryOpaqueCaptureAddressAllocateInfo s_MemoryOpaqueCaptureAddress;
  static thread_local VkMemoryAllocateFlagsInfo s_MemoryAllocateFlags;
  // Scratch used until EncoderLayer serializes the image create command.
  static thread_local std::vector<uint8_t> s_OpaqueData;
  static thread_local VkHostAddressRangeConstEXT s_OpaqueDataRange;
  static thread_local VkOpaqueCaptureDataCreateInfoEXT s_OpaqueCreateInfo;
};

} // namespace vulkan
} // namespace gits
