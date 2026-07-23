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

namespace gits {
namespace vulkan {

class RayTracingCaptureService {
public:
  RayTracingCaptureService(CaptureManager& manager) : m_Manager(manager) {}

  void OnPreCreateDevice(vkCreateDeviceCommand& command);
  void OnPostCreateDevice(vkCreateDeviceCommand& command);
  void OnPreCreateBuffer(vkCreateBufferCommand& command);
  void OnPostCreateBuffer(vkCreateBufferCommand& command);
  void OnPreAllocateMemory(vkAllocateMemoryCommand& command);
  void OnPostAllocateMemory(vkAllocateMemoryCommand& command);
  void OnPostCreateAccelerationStructureKHR(vkCreateAccelerationStructureKHRCommand& command);
  void OnPostCreateMicromapEXT(vkCreateMicromapEXTCommand& command);
  void OnPreCreateRayTracingPipelinesKHR(vkCreateRayTracingPipelinesKHRCommand& command);
  void OnPostCreateRayTracingPipelinesKHR(vkCreateRayTracingPipelinesKHRCommand& command);

private:
  void GetPhysicalDeviceCapabilities(HandleArgument<VkPhysicalDevice>& physicalDevice);

  struct RayTracingCapabilities {
    bool m_BufferDeviceAddressCaptureReplay;
    bool m_AccelerationStructureCaptureReplay;
    bool m_RayTracingPipelineShaderGroupHandleCaptureReplay;
    uint32_t m_ShaderGroupCaptureReplayHandleSize;
  };

  CaptureManager& m_Manager;
  std::map<GITSKey, RayTracingCapabilities> m_Caps; // Both per physical device and per device
  static thread_local VkBufferOpaqueCaptureAddressCreateInfo s_BufferOpaqueCaptureAddress;
  static thread_local VkMemoryOpaqueCaptureAddressAllocateInfo s_MemoryOpaqueCaptureAddress;
  static thread_local RayTracingCapabilities s_DeviceCaps;
}; // class RayTracingCaptureService

} // namespace vulkan
} // namespace gits
