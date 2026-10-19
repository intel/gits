// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "pipelineTracker.h"
#include "rtsr/shared/client/rtsrClient.h"
#include "vulkanContext.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

class Timer;

namespace gits {

class CGits;

namespace vulkan {

class PerformanceTracker {
public:
  PerformanceTracker(rtsr::RtsrClient& client,
                     PipelineTracker& pipelines,
                     RtsrContextRegistry& contexts,
                     CGits* gits,
                     bool playback);
  ~PerformanceTracker();

  PerformanceTracker(const PerformanceTracker&) = delete;
  PerformanceTracker& operator=(const PerformanceTracker&) = delete;

  rtsr::RTSR_RESULT SetEnabled(bool enable);
  bool IsActive() const;

  void OnGetDeviceQueue(vkGetDeviceQueueCommand& command);
  void OnGetDeviceQueue2(vkGetDeviceQueue2Command& command);
  void OnCreateCommandPool(vkCreateCommandPoolCommand& command);
  void OnDestroyCommandPool(vkDestroyCommandPoolCommand& command);
  void OnResetCommandPool(vkResetCommandPoolCommand& command);
  void OnAllocateCommandBuffers(vkAllocateCommandBuffersCommand& command);
  void OnFreeCommandBuffers(vkFreeCommandBuffersCommand& command);
  void OnResetCommandBuffer(vkResetCommandBufferCommand& command);
  void OnBeginCommandBuffer(vkBeginCommandBufferCommand& command);
  void OnEndCommandBufferPre(vkEndCommandBufferCommand& command);
  void OnEndCommandBufferPost(vkEndCommandBufferCommand& command);
  void OnBindPipeline(vkCmdBindPipelineCommand& command);
  void OnWorkPre(GITSKey commandBufferKey, VkPipelineBindPoint bindPoint);
  void OnWorkPost(GITSKey commandBufferKey);
  void TrackRenderPass(GITSKey deviceKey,
                       VkRenderPass renderPass,
                       const VkRenderPassCreateInfo& info);
  void TrackRenderPass(GITSKey deviceKey,
                       VkRenderPass renderPass,
                       const VkRenderPassCreateInfo2& info);
  void ForgetRenderPass(GITSKey deviceKey, VkRenderPass renderPass);
  void BeginRenderPass(GITSKey commandBufferKey, VkRenderPass renderPass);
  void NextSubpass(GITSKey commandBufferKey);
  void BeginRendering(GITSKey commandBufferKey, uint32_t viewMask);
  void EndRendering(GITSKey commandBufferKey);
  void OnExecuteCommands(vkCmdExecuteCommandsCommand& command);
  void OnQueueSubmit(vkQueueSubmitCommand& command);
  void OnQueueSubmit2(vkQueueSubmit2Command& command);
  void OnQueueSubmit2(vkQueueSubmit2KHRCommand& command);
  void OnQueueSubmitPost(VkQueue queue, VkResult result);
  void OnPresent();
  void PrepareDeviceDestroy(GITSKey deviceKey);
  void OnDestroyDevice(GITSKey deviceKey);

private:
  struct Segment {
    GITSKey PipelineKey{};
    uint32_t Begin{};
    uint32_t End{};
  };

  struct Recording {
    std::mutex LifetimeMutex;
    std::shared_ptr<RtsrDeviceContext> Device;
    VkQueryPool QueryPool{VK_NULL_HANDLE};
    uint32_t QueueFamily{};
    uint32_t QueryCount{};
    uint32_t EndQuery{};
    std::vector<Segment> Segments;
    GITSKey GraphicsPipeline{};
    GITSKey ComputePipeline{};
    std::optional<Segment> PendingWork;
    std::vector<std::shared_ptr<Recording>> Secondaries;
    std::atomic<bool> Submitted{false};
    std::atomic<size_t> PendingReadbacks{0};
    std::atomic<bool> Retired{false};
    std::atomic<bool> Destroyed{false};
    std::atomic<bool> Unresolved{false};
    bool Closed{false};
    bool CompleteCoverage{true};
    bool SingleView{true};
    uint32_t Subpass{};
    std::vector<uint32_t> ViewMasks;
  };

  struct CommandPool {
    GITSKey DeviceKey{};
    VkCommandPool Handle{VK_NULL_HANDLE};
    uint32_t QueueFamily{};
    VkCommandPoolCreateFlags Flags{};
  };

  struct CommandBuffer {
    GITSKey Key{};
    VkCommandBuffer Handle{VK_NULL_HANDLE};
    GITSKey DeviceKey{};
    VkCommandPool Pool{VK_NULL_HANDLE};
    uint32_t QueueFamily{};
    VkCommandBufferLevel Level{VK_COMMAND_BUFFER_LEVEL_PRIMARY};
    VkCommandPoolCreateFlags PoolFlags{};
    std::shared_ptr<Recording> Current;
  };

  struct Queue {
    std::shared_ptr<RtsrDeviceContext> Device;
    uint32_t Family{};
  };

  struct Frame {
    size_t Number{};
    size_t DurationNs{};
    bool Closed{false};
    bool Dropped{false};
    size_t Pending{};
    std::vector<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> CommandBuffers;
  };

  struct Readback {
    std::shared_ptr<RtsrDeviceContext> Device;
    VkFence Fence{VK_NULL_HANDLE};
    size_t FrameNumber{};
    std::vector<std::shared_ptr<Recording>> Recordings;
  };

  static constexpr uint32_t MAX_SEGMENTS = 4096;
  static constexpr uint32_t QUERY_CAPACITY = MAX_SEGMENTS * 2 + 2;
  static constexpr size_t MAX_LIVE_RECORDINGS = 1024;
  static constexpr size_t MAX_PENDING_FRAMES = 64;

  void Submit(VkQueue queue, const std::vector<VkCommandBuffer>& commandBuffers);
  void SubmitRecordings(VkQueue queue, std::vector<std::shared_ptr<Recording>> recordings);
  void CollectRecording(const std::shared_ptr<Recording>& recording,
                        std::vector<std::shared_ptr<Recording>>& recordings) const;
  void RetireRecording(const std::shared_ptr<Recording>& recording, bool forceDestroy = false);
  void DestroyRecording(const std::shared_ptr<Recording>& recording,
                        bool operationHeld,
                        bool forceDestroy);
  void ForgetCommandBufferLocked(GITSKey key, std::vector<std::shared_ptr<Recording>>& retired);
  int64_t NowNs() const;
  void Worker();
  void ReadBack(Readback readback);
  std::optional<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> Summarize(
      const Recording& recording) const;
  void TakeCompleteFrames(std::vector<Frame>& complete);
  void Send(std::vector<Frame> frames);

  rtsr::RtsrClient& m_Client;
  PipelineTracker& m_Pipelines;
  RtsrContextRegistry& m_Contexts;
  Timer* m_PlaybackClock{};
  std::chrono::steady_clock::time_point m_WallClockOrigin;

  std::atomic<bool> m_Collecting{false};
  std::atomic<size_t> m_LiveRecordings{0};

  mutable std::mutex m_Mutex;
  std::unordered_map<GITSKey, std::unordered_map<VkCommandPool, CommandPool>> m_CommandPools;
  std::unordered_map<GITSKey, CommandBuffer> m_CommandBuffers;
  std::unordered_map<VkCommandBuffer, GITSKey> m_CommandBufferKeys;
  std::unordered_map<VkQueue, Queue> m_Queues;
  std::unordered_map<VkQueue, std::vector<std::shared_ptr<Recording>>> m_Submissions;
  std::unordered_map<GITSKey, std::unordered_map<VkRenderPass, std::vector<uint32_t>>> m_ViewMasks;
  std::unordered_map<GITSKey, std::vector<VkFence>> m_UnresolvedFences;
  std::unordered_map<GITSKey, std::vector<std::shared_ptr<Recording>>> m_UnresolvedRecordings;
  std::deque<Frame> m_Frames;
  size_t m_FrameNumber{};
  bool m_NeedBaseline{true};
  bool m_FrameOpen{false};
  int64_t m_LastPresentNs{};

  std::mutex m_WorkMutex;
  std::condition_variable m_WorkAvailable;
  std::deque<Readback> m_Readbacks;
  bool m_Stopping{false};
  std::thread m_Worker;
};

} // namespace vulkan
} // namespace gits
