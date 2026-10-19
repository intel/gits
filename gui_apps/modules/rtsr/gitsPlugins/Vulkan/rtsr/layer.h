// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "performanceTracker.h"
#include "pipelineTracker.h"
#include "rtsrPluginConfig.h"
#include "rtsr/shared/client/rtsrClient.h"
#include "shaderModuleTracker.h"
#include "vulkanContext.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits {

class CGits;

namespace vulkan {

class DispatchTablesHolder;

class RtsrLayer : public Layer {
public:
  RtsrLayer(const rtsr::PluginConfig& config,
            CGits* gits,
            bool playback,
            bool execute,
            DispatchTablesHolder* dispatchTables);
  ~RtsrLayer() override;

  void Pre(vkCreateInstanceCommand& command) override;
  void Post(vkCreateInstanceCommand& command) override;
  void Pre(vkDestroyInstanceCommand& command) override;
  void Post(vkDestroyInstanceCommand& command) override;
  void Pre(vkCreateDeviceCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
  void Pre(vkDestroyDeviceCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkSetDebugUtilsObjectNameEXTCommand& command) override;

  void Post(vkCreateShaderModuleCommand& command) override;
  void Post(vkDestroyShaderModuleCommand& command) override;
  void Pre(vkGetShaderModuleIdentifierEXTCommand& command) override;
  void Post(vkGetShaderModuleIdentifierEXTCommand& command) override;
  void Pre(vkGetShaderModuleCreateInfoIdentifierEXTCommand& command) override;
  void Post(vkGetShaderModuleCreateInfoIdentifierEXTCommand& command) override;

  void Post(vkCreateGraphicsPipelinesCommand& command) override;
  void Post(vkCreateComputePipelinesCommand& command) override;
  void Post(vkCreatePipelineLayoutCommand& command) override;
  void Post(vkCreateRenderPassCommand& command) override;
  void Post(vkCreateRenderPass2Command& command) override;
  void Post(vkCreateRenderPass2KHRCommand& command) override;
  void Post(vkDestroyPipelineCommand& command) override;
  void Pre(vkDestroyPipelineLayoutCommand& command) override;
  void Pre(vkDestroyRenderPassCommand& command) override;
  void Pre(vkCmdBindPipelineCommand& command) override;
  void Post(vkCmdBeginRenderPassCommand& command) override;
  void Post(vkCmdBeginRenderPass2Command& command) override;
  void Post(vkCmdBeginRenderPass2KHRCommand& command) override;
  void Post(vkCmdNextSubpassCommand& command) override;
  void Post(vkCmdNextSubpass2Command& command) override;
  void Post(vkCmdNextSubpass2KHRCommand& command) override;
  void Post(vkCmdEndRenderPassCommand& command) override;
  void Post(vkCmdEndRenderPass2Command& command) override;
  void Post(vkCmdEndRenderPass2KHRCommand& command) override;
  void Post(vkCmdBeginRenderingCommand& command) override;
  void Post(vkCmdBeginRenderingKHRCommand& command) override;
  void Post(vkCmdEndRenderingCommand& command) override;
  void Post(vkCmdEndRenderingKHRCommand& command) override;

#define RTSR_DECLARE_WORK_HOOKS(commandType)                                                       \
  void Pre(commandType& command) override;                                                         \
  void Post(commandType& command) override
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndexedCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawMultiEXTCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawMultiIndexedEXTCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndirectCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndexedIndirectCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndirectCountCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndirectCountKHRCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndirectCountAMDCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountKHRCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountAMDCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDrawIndirectByteCountEXTCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDispatchCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDispatchIndirectCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDispatchBaseCommand);
  RTSR_DECLARE_WORK_HOOKS(vkCmdDispatchBaseKHRCommand);
#undef RTSR_DECLARE_WORK_HOOKS

  void Post(vkCreateCommandPoolCommand& command) override;
  void Post(vkDestroyCommandPoolCommand& command) override;
  void Post(vkResetCommandPoolCommand& command) override;
  void Post(vkAllocateCommandBuffersCommand& command) override;
  void Post(vkFreeCommandBuffersCommand& command) override;
  void Post(vkResetCommandBufferCommand& command) override;
  void Post(vkBeginCommandBufferCommand& command) override;
  void Pre(vkEndCommandBufferCommand& command) override;
  void Post(vkEndCommandBufferCommand& command) override;
  void Pre(vkCmdExecuteCommandsCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;
  void Pre(vkQueueSubmitCommand& command) override;
  void Post(vkQueueSubmit2Command& command) override;
  void Pre(vkQueueSubmit2Command& command) override;
  void Post(vkQueueSubmit2KHRCommand& command) override;
  void Pre(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;

  void Post(StateRestoreBeginCommand& command) override;
  void Post(StateRestoreEndCommand& command) override;

private:
  struct PendingInstance {
    GITSKey Key{};
    VkInstance Handle{VK_NULL_HANDLE};
  };

  struct PendingDevice {
    GITSKey Key{};
    GITSKey PhysicalDeviceKey{};
    VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
    VkDevice Handle{VK_NULL_HANDLE};
  };

  void FlushPendingContexts();
  void Connect(const vkCreateInstanceCommand& command);

  rtsr::PluginConfig m_Config;
  bool m_Playback{};
  bool m_Available{false};
  std::atomic<bool> m_Enabled{false};
  std::atomic<bool> m_StateRestore{false};

  DispatchTablesHolder* m_DispatchTables{};
  std::unordered_map<GITSKey, PendingInstance> m_PendingInstances;
  std::unordered_map<GITSKey, PendingDevice> m_PendingDevices;
  std::unordered_set<GITSKey> m_LiveInstances;
  std::mutex m_LifecycleMutex;
  std::condition_variable m_ConnectFinished;
  bool m_ConnectAttempted{false};
  bool m_ConnectInProgress{false};

  RtsrContextRegistry m_Contexts;
  std::unique_ptr<rtsr::RtsrClient> m_Client;
  std::unique_ptr<ShaderModuleTracker> m_ShaderModules;
  std::unique_ptr<PipelineTracker> m_Pipelines;
  std::unique_ptr<PerformanceTracker> m_Performance;
};

} // namespace vulkan
} // namespace gits
