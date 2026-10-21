// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "commandsAuto.h"
#include "pipelineStateSnapshot.h"
#include "rtsr/shared/client/rtsrClient.h"
#include "shaderModuleTracker.h"
#include "vulkanContext.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace gits::vulkan {

class PipelineTracker {
public:
  PipelineTracker(rtsr::RtsrClient& client,
                  RtsrContextRegistry& contexts,
                  ShaderModuleTracker& shaders);
  ~PipelineTracker();

  void HarvestGraphics(vkCreateGraphicsPipelinesCommand& command);
  void HarvestCompute(vkCreateComputePipelinesCommand& command);
  void OnCreatePipelineLayout(GITSKey deviceKey,
                              GITSKey key,
                              VkPipelineLayout layout,
                              VkResult result);
  void OnCreateRenderPass(GITSKey deviceKey, GITSKey key, VkRenderPass renderPass, VkResult result);
  // Forwards an application debug name. RTSR does not invent one.
  void OnSetName(GITSKey deviceKey, const VkDebugUtilsObjectNameInfoEXT* nameInfo);
  void OnDestroy(GITSKey deviceKey, GITSKey key, VkPipeline pipeline);
  void OnDestroyPipelineLayout(GITSKey deviceKey, VkPipelineLayout layout);
  void OnDestroyRenderPass(GITSKey deviceKey, VkRenderPass renderPass);
  void OnDestroyDevice(GITSKey deviceKey);

  void OnAllocateCommandBuffers(vkAllocateCommandBuffersCommand& command);
  void OnFreeCommandBuffers(vkFreeCommandBuffersCommand& command);
  void OnResetCommandBuffer(GITSKey key, VkResult result);
  void OnResetCommandPool(GITSKey deviceKey, VkCommandPool pool, bool destroy);
  VkPipeline OnBind(GITSKey key, VkPipeline pipeline, GITSKey commandBufferKey);
  bool IsTracked(GITSKey key) const;
  bool HasReplacementWork() const {
    return m_ActiveReplacements.load(std::memory_order_relaxed) != 0;
  }

  rtsr::RTSR_RESULT ApplyUpdates(const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request);
  rtsr::RTSR_RESULT ResetPipelines(const rtsr::RTSR_RESET_PIPELINES_REQUEST& request);

private:
  struct ReplacementPipeline {
    VkPipeline Handle{VK_NULL_HANDLE};
    bool UntrackedUse{};
  };

  struct CommandBuffer {
    GITSKey DeviceKey{};
    VkCommandPool Pool{VK_NULL_HANDLE};
    std::vector<std::shared_ptr<ReplacementPipeline>> Replacements;
  };

  struct TrackedPipeline {
    GITSKey Key{};
    std::shared_ptr<RtsrDeviceContext> Device;
    VkPipeline Original{VK_NULL_HANDLE};
    std::shared_ptr<ReplacementPipeline> Replacement;
    VkPipelineLayout Layout{VK_NULL_HANDLE};
    VkRenderPass RenderPass{VK_NULL_HANDLE};
    rtsr::RTSR_PIPELINE_TYPE Type{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
    rtsr::RTSR_HASH StateHash{};
    rtsr::RTSR_PIPELINE_INCARNATION Incarnation{};
    std::string Name;
    std::shared_ptr<const PipelineStateSnapshot> Snapshot;
    bool ReplacementEligible{false};
    std::string Diagnostic;

    TrackedPipeline(GITSKey key,
                    std::shared_ptr<RtsrDeviceContext> device,
                    VkPipeline original,
                    VkPipelineLayout layout,
                    VkRenderPass renderPass,
                    rtsr::RTSR_PIPELINE_TYPE type,
                    rtsr::RTSR_HASH stateHash,
                    std::string name,
                    std::shared_ptr<const PipelineStateSnapshot> snapshot,
                    bool replacementEligible,
                    std::string diagnostic)
        : Key(key),
          Device(std::move(device)),
          Original(original),
          Layout(layout),
          RenderPass(renderPass),
          Type(type),
          StateHash(stateHash),
          Name(std::move(name)),
          Snapshot(std::move(snapshot)),
          ReplacementEligible(replacementEligible),
          Diagnostic(std::move(diagnostic)) {}
  };

  struct ReplacementInputs {
    GITSKey Key{};
    std::shared_ptr<RtsrDeviceContext> Device;
    rtsr::RTSR_PIPELINE_TYPE Type{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
    rtsr::RTSR_HASH StateHash{};
    rtsr::RTSR_PIPELINE_INCARNATION Incarnation{};
    std::string Name;
    std::shared_ptr<const PipelineStateSnapshot> Snapshot;
    bool Eligible{false};
    std::string Diagnostic;
  };

  struct RetiredPipelines {
    std::shared_ptr<RtsrDeviceContext> Device;
    std::vector<std::shared_ptr<ReplacementPipeline>> Pipelines;
  };

  void Publish(std::unique_ptr<TrackedPipeline> pipeline,
               std::string state,
               std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages);
  void DestroyReplacement(TrackedPipeline& pipeline);
  void CollectRetiredLocked(GITSKey deviceKey);
  size_t ReplacementCountLocked(GITSKey deviceKey) const;
  ReplacementInputs SnapshotForReplacement(GITSKey key) const;
  bool InstallReplacement(GITSKey key,
                          rtsr::RTSR_HASH stateHash,
                          rtsr::RTSR_PIPELINE_INCARNATION incarnation,
                          VkPipeline replacement);

  rtsr::RtsrClient& m_Client;
  RtsrContextRegistry& m_Contexts;
  ShaderModuleTracker& m_Shaders;

  mutable std::mutex m_Mutex;
  std::unordered_map<GITSKey, std::unique_ptr<TrackedPipeline>> m_Pipelines;
  std::unordered_map<GITSKey, std::unordered_map<VkPipeline, GITSKey>> m_PipelineKeys;
  // Debug names recorded before the pipeline is harvested.
  std::unordered_map<GITSKey, std::unordered_map<VkPipeline, std::string>> m_PendingNames;
  std::unordered_map<GITSKey, std::unordered_map<VkPipelineLayout, GITSKey>> m_PipelineLayoutKeys;
  std::unordered_map<GITSKey, std::unordered_map<VkRenderPass, GITSKey>> m_RenderPassKeys;
  std::unordered_map<GITSKey, RetiredPipelines> m_Retired;
  std::unordered_map<GITSKey, CommandBuffer> m_CommandBuffers;
  std::atomic<size_t> m_ActiveReplacements{0};
  rtsr::RTSR_PIPELINE_INCARNATION m_NextIncarnation{1};
};

} // namespace gits::vulkan
