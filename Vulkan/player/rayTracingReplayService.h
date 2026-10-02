// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "dispatchTableAuto.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

class PlayerManager;

// Takes care of applying capture/replay handles to ray tracing pipelines
// and patches Shader Binding Tables (depending on selected options).
class RayTracingReplayService {
public:
  RayTracingReplayService(PlayerManager& manager) : m_Manager(manager) {}

  void OnPreCreateDevice(vkCreateDeviceCommand& command);
  void OnPostCreateDevice(vkCreateDeviceCommand& command);
  void OnPreDestroyDevice(vkDestroyDeviceCommand& command);
  void OnPostAllocateCommandBuffers(vkAllocateCommandBuffersCommand& command);
  void OnPreFreeCommandBuffers(vkFreeCommandBuffersCommand& command);
  void OnPreDestroyCommandPool(vkDestroyCommandPoolCommand& command);
  void OnPostBindPipeline(vkCmdBindPipelineCommand& command);
  void OnPostPushConstants(vkCmdPushConstantsCommand& command);
  void OnPreCreateRayTracingPipelines(vkCreateRayTracingPipelinesKHRCommand& command);
  void OnPostCreateRayTracingPipelines(vkCreateRayTracingPipelinesKHRCommand& command);
  void OnPostDestroyPipeline(vkDestroyPipelineCommand& command);
  void OnPreGetShaderGroupHandles(vkGetRayTracingShaderGroupHandlesKHRCommand& command);
  void OnPostGetShaderGroupHandles(vkGetRayTracingShaderGroupHandlesKHRCommand& command);
  void OnPreTraceRays(vkCmdTraceRaysKHRCommand& command);
  void OnPostTraceRays(vkCmdTraceRaysKHRCommand& command);
  void OnPreTraceRaysIndirect(vkCmdTraceRaysIndirectKHRCommand& command);
  void OnPostTraceRaysIndirect(vkCmdTraceRaysIndirectKHRCommand& command);

private:
  static const uint32_t s_ShaderGroupHandleSize = 32;

  struct CommandBufferData {
    VkDevice m_Device;
    VkPipelineBindPoint m_BindPoint;
    VkPipeline m_Pipeline;
    GITSKey m_PipelineKey;

    struct PushConstantData {
      VkPipelineLayout m_Layout;
      VkShaderStageFlags m_StageFlags;
      uint32_t m_Offset;
      uint32_t m_Size;
      std::vector<uint8_t> m_Data;
    } m_PushConstants;
  };

  struct BufferData {
    VkDeviceMemory m_Memory;
    VkBuffer m_Buffer;
    VkDeviceSize m_Size;
    VkDeviceAddress m_DeviceAddress;
  };

  struct PipelineData {
    uint32_t m_TotalGroupCount;
    std::vector<uint8_t> m_OriginalHandles;
    std::vector<uint8_t> m_ReplayHandles;
    std::vector<bool> m_LoadedGroups;
    bool m_PatchingRequired;
    bool m_AlreadyProcessed;
    BufferData m_Buffer;
  };

  struct DeviceData {
    std::unordered_map<VkCommandPool, std::unordered_set<VkCommandBuffer>> m_CommandBuffers;
    std::unordered_map<VkPipeline, uint32_t> m_GroupCounts;
    VkPhysicalDevice m_PhysicalDevice;
    VkPipelineLayout m_Layout;
    VkPipeline m_ComputePipeline;
  };

  PlayerManager& m_Manager;
  std::unordered_map<VkDevice, DeviceData> m_DevicesData;
  std::unordered_map<GITSKey, PipelineData> m_PipelinesData;
  std::unordered_map<VkCommandBuffer, CommandBufferData> m_CommandBuffersData;
  static thread_local std::vector<uint8_t> tl_ShaderGroupHandles;

  VkPipelineLayout CreatePipelineLayout(VkDevice device);
  VkPipeline CreateComputePipeline(VkDevice device, VkPipelineLayout layout);
  BufferData CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size);
  bool CopyHandles(vkGetRayTracingShaderGroupHandlesKHRCommand& command,
                   uint32_t totalGroupCount,
                   uint8_t* target);
  void PatchSBT(VkCommandBuffer cmdBuf,
                const VkStridedDeviceAddressRegionKHR* pRaygenSBT,
                const VkStridedDeviceAddressRegionKHR* pMissSBT,
                const VkStridedDeviceAddressRegionKHR* pHitSBT,
                const VkStridedDeviceAddressRegionKHR* pCallableSBT,
                VkDeviceAddress oldHandlesMap,
                VkDeviceAddress newHandlesMap,
                uint32_t handlesMapEntriesCount);
}; // class RayTracingReplayService

} // namespace vulkan
} // namespace gits
