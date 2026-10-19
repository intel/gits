// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "performanceTracker.h"

#include "gits.h"
#include "log.h"
#include "timer.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace gits::vulkan {
namespace {

class DeviceOperation {
public:
  explicit DeviceOperation(std::shared_ptr<RtsrDeviceContext> device)
      : m_Device(std::move(device)), m_Active(m_Device && m_Device->BeginOperation()) {}
  ~DeviceOperation() {
    if (m_Active) {
      m_Device->EndOperation();
    }
  }
  explicit operator bool() const {
    return m_Active;
  }

private:
  std::shared_ptr<RtsrDeviceContext> m_Device;
  bool m_Active{};
};

uint64_t TimestampDelta(uint64_t begin, uint64_t end, uint32_t validBits) {
  if (validBits == 0) {
    return 0;
  }
  if (validBits >= 64) {
    return end - begin;
  }
  const uint64_t mask = (uint64_t{1} << validBits) - 1;
  return (end - begin) & mask;
}

size_t Nanoseconds(uint64_t ticks, float timestampPeriod) {
  const long double value =
      static_cast<long double>(ticks) * static_cast<long double>(timestampPeriod);
  if (value <= 0.0L) {
    return 0;
  }
  const long double maximum = static_cast<long double>(std::numeric_limits<size_t>::max());
  return value >= maximum ? std::numeric_limits<size_t>::max() : static_cast<size_t>(value + 0.5L);
}

} // namespace

PerformanceTracker::PerformanceTracker(rtsr::RtsrClient& client,
                                       PipelineTracker& pipelines,
                                       RtsrContextRegistry& contexts,
                                       CGits* gits,
                                       bool playback)
    : m_Client(client),
      m_Pipelines(pipelines),
      m_Contexts(contexts),
      m_PlaybackClock(playback && gits != nullptr ? &gits->Timers().playback : nullptr),
      m_WallClockOrigin(std::chrono::steady_clock::now()),
      m_Worker([this] { Worker(); }) {}

PerformanceTracker::~PerformanceTracker() {
  m_Collecting.store(false);
  std::vector<GITSKey> devices;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& entry : m_CommandBuffers) {
      if (std::find(devices.begin(), devices.end(), entry.second.DeviceKey) == devices.end()) {
        devices.push_back(entry.second.DeviceKey);
      }
    }
    for (const auto& entry : m_Queues) {
      if (std::find(devices.begin(), devices.end(), entry.second.Device->Key) == devices.end()) {
        devices.push_back(entry.second.Device->Key);
      }
    }
    for (const auto& entry : m_UnresolvedRecordings) {
      if (std::find(devices.begin(), devices.end(), entry.first) == devices.end()) {
        devices.push_back(entry.first);
      }
    }
  }
  for (const GITSKey deviceKey : devices) {
    PrepareDeviceDestroy(deviceKey);
  }

  {
    std::lock_guard<std::mutex> lock(m_WorkMutex);
    m_Stopping = true;
  }
  m_WorkAvailable.notify_all();
  if (m_Worker.joinable()) {
    m_Worker.join();
  }
  for (const GITSKey deviceKey : devices) {
    OnDestroyDevice(deviceKey);
  }
}

rtsr::RTSR_RESULT PerformanceTracker::SetEnabled(bool enable) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (enable == m_Collecting.load()) {
    return rtsr::RTSR_RESULT_SUCCESS;
  }
  if (enable) {
    m_NeedBaseline = true;
    m_Collecting.store(true);
    LOG_INFO << "Rtsr - Vulkan pipeline timing starts at the next present";
  } else {
    m_Collecting.store(false);
    m_NeedBaseline = false;
    if (m_FrameOpen && !m_Frames.empty()) {
      m_Frames.pop_back();
      m_FrameOpen = false;
    }
    LOG_INFO << "Rtsr - Vulkan pipeline timing disabled";
  }
  return rtsr::RTSR_RESULT_SUCCESS;
}

bool PerformanceTracker::IsActive() const {
  return m_Collecting.load(std::memory_order_relaxed) ||
         m_LiveRecordings.load(std::memory_order_relaxed) != 0;
}

void PerformanceTracker::OnGetDeviceQueue(vkGetDeviceQueueCommand& command) {
  if (command.m_pQueue.Value == nullptr || *command.m_pQueue.Value == VK_NULL_HANDLE) {
    return;
  }
  auto device = m_Contexts.Device(command.m_device.Key);
  if (!device) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Queues[*command.m_pQueue.Value] = Queue{std::move(device), command.m_queueFamilyIndex.Value};
}

void PerformanceTracker::OnGetDeviceQueue2(vkGetDeviceQueue2Command& command) {
  if (command.m_pQueueInfo.Value == nullptr || command.m_pQueue.Value == nullptr ||
      *command.m_pQueue.Value == VK_NULL_HANDLE) {
    return;
  }
  auto device = m_Contexts.Device(command.m_device.Key);
  if (!device) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Queues[*command.m_pQueue.Value] =
      Queue{std::move(device), command.m_pQueueInfo.Value->queueFamilyIndex};
}

void PerformanceTracker::OnCreateCommandPool(vkCreateCommandPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || command.m_pCreateInfo.Value == nullptr ||
      command.m_pCommandPool.Value == nullptr || *command.m_pCommandPool.Value == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CommandPools[command.m_device.Key][*command.m_pCommandPool.Value] = CommandPool{
      command.m_device.Key, *command.m_pCommandPool.Value,
      command.m_pCreateInfo.Value->queueFamilyIndex, command.m_pCreateInfo.Value->flags};
}

void PerformanceTracker::OnDestroyCommandPool(vkDestroyCommandPoolCommand& command) {
  std::vector<std::shared_ptr<Recording>> retired;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto buffer = m_CommandBuffers.begin(); buffer != m_CommandBuffers.end();) {
      if (buffer->second.DeviceKey == command.m_device.Key &&
          buffer->second.Pool == command.m_commandPool.Value) {
        if (buffer->second.Current) {
          retired.push_back(std::move(buffer->second.Current));
        }
        m_CommandBufferKeys.erase(buffer->second.Handle);
        buffer = m_CommandBuffers.erase(buffer);
      } else {
        ++buffer;
      }
    }
    const auto pools = m_CommandPools.find(command.m_device.Key);
    if (pools != m_CommandPools.end()) {
      pools->second.erase(command.m_commandPool.Value);
    }
  }
  for (const auto& recording : retired) {
    RetireRecording(recording);
  }
}

void PerformanceTracker::OnResetCommandPool(vkResetCommandPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  std::vector<std::shared_ptr<Recording>> retired;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& entry : m_CommandBuffers) {
      if (entry.second.DeviceKey == command.m_device.Key &&
          entry.second.Pool == command.m_commandPool.Value && entry.second.Current) {
        retired.push_back(std::move(entry.second.Current));
      }
    }
  }
  for (const auto& recording : retired) {
    RetireRecording(recording);
  }
}

void PerformanceTracker::OnAllocateCommandBuffers(vkAllocateCommandBuffersCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || command.m_pAllocateInfo.Value == nullptr ||
      command.m_pCommandBuffers.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto pools = m_CommandPools.find(command.m_device.Key);
  if (pools == m_CommandPools.end()) {
    return;
  }
  const auto pool = pools->second.find(command.m_pAllocateInfo.Value->commandPool);
  if (pool == pools->second.end()) {
    return;
  }
  const uint32_t count =
      std::min({command.m_pAllocateInfo.Value->commandBufferCount, command.m_pCommandBuffers.Size,
                static_cast<uint32_t>(command.m_pCommandBuffers.Keys.size())});
  for (uint32_t i = 0; i < count; ++i) {
    const GITSKey key = command.m_pCommandBuffers.Keys[i];
    const VkCommandBuffer handle = command.m_pCommandBuffers.Value[i];
    if (key == 0 || handle == VK_NULL_HANDLE) {
      continue;
    }
    m_CommandBuffers[key] = CommandBuffer{key,
                                          handle,
                                          command.m_device.Key,
                                          pool->second.Handle,
                                          pool->second.QueueFamily,
                                          command.m_pAllocateInfo.Value->level,
                                          pool->second.Flags};
    m_CommandBufferKeys[handle] = key;
  }
}

void PerformanceTracker::OnFreeCommandBuffers(vkFreeCommandBuffersCommand& command) {
  std::vector<std::shared_ptr<Recording>> retired;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (uint32_t i = 0;
         command.m_pCommandBuffers.Value != nullptr && i < command.m_pCommandBuffers.Size; ++i) {
      const auto key = m_CommandBufferKeys.find(command.m_pCommandBuffers.Value[i]);
      if (key != m_CommandBufferKeys.end()) {
        ForgetCommandBufferLocked(key->second, retired);
      }
    }
  }
  for (const auto& recording : retired) {
    RetireRecording(recording);
  }
}

void PerformanceTracker::OnResetCommandBuffer(vkResetCommandBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  std::shared_ptr<Recording> retired;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto buffer = m_CommandBuffers.find(command.m_commandBuffer.Key);
    if (buffer != m_CommandBuffers.end()) {
      retired = std::move(buffer->second.Current);
    }
  }
  RetireRecording(retired);
}

void PerformanceTracker::OnBeginCommandBuffer(vkBeginCommandBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  CommandBuffer buffer;
  std::shared_ptr<Recording> retired;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto found = m_CommandBuffers.find(command.m_commandBuffer.Key);
    if (found == m_CommandBuffers.end()) {
      return;
    }
    retired = std::move(found->second.Current);
    buffer = found->second;
  }
  RetireRecording(retired);

  const VkCommandBufferUsageFlags flags =
      command.m_pBeginInfo.Value != nullptr ? command.m_pBeginInfo.Value->flags : 0;
  if (!m_Collecting.load(std::memory_order_relaxed) ||
      (flags & (VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
                VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) != 0 ||
      (flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) == 0 ||
      (buffer.PoolFlags & VK_COMMAND_POOL_CREATE_PROTECTED_BIT) != 0) {
    return;
  }

  size_t live = m_LiveRecordings.load(std::memory_order_relaxed);
  while (live < MAX_LIVE_RECORDINGS &&
         !m_LiveRecordings.compare_exchange_weak(live, live + 1, std::memory_order_relaxed)) {
  }
  if (live >= MAX_LIVE_RECORDINGS) {
    return;
  }
  const auto releaseReservation = [this]() {
    m_LiveRecordings.fetch_sub(1, std::memory_order_relaxed);
  };

  auto device = m_Contexts.Device(buffer.DeviceKey);
  if (!device || buffer.QueueFamily >= device->TimestampValidBits.size() ||
      buffer.QueueFamily >= device->QueueFlags.size() ||
      (device->QueueFlags[buffer.QueueFamily] & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
          0 ||
      device->TimestampValidBits[buffer.QueueFamily] == 0 || device->TimestampPeriod <= 0.0f ||
      device->Dispatch.vkCreateQueryPool == nullptr ||
      device->Dispatch.vkCmdResetQueryPool == nullptr ||
      device->Dispatch.vkCmdWriteTimestamp == nullptr) {
    releaseReservation();
    return;
  }

  DeviceOperation operation(device);
  if (!operation) {
    releaseReservation();
    return;
  }
  VkQueryPoolCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  createInfo.queryCount = QUERY_CAPACITY;
  VkQueryPool queryPool = VK_NULL_HANDLE;
  if (device->Dispatch.vkCreateQueryPool(device->Handle, &createInfo, nullptr, &queryPool) !=
          VK_SUCCESS ||
      queryPool == VK_NULL_HANDLE) {
    releaseReservation();
    return;
  }

  device->Dispatch.vkCmdResetQueryPool(buffer.Handle, queryPool, 0, QUERY_CAPACITY);
  device->Dispatch.vkCmdWriteTimestamp(buffer.Handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool,
                                       0);

  auto recording = std::make_shared<Recording>();
  recording->Device = std::move(device);
  recording->QueryPool = queryPool;
  recording->QueueFamily = buffer.QueueFamily;
  recording->QueryCount = 1;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto found = m_CommandBuffers.find(command.m_commandBuffer.Key);
    if (found == m_CommandBuffers.end() || found->second.Current) {
      recording->Retired.store(true);
    } else {
      found->second.Current = recording;
      return;
    }
  }
  releaseReservation();
  DestroyRecording(recording, true, true);
}

void PerformanceTracker::OnEndCommandBufferPre(vkEndCommandBufferCommand& command) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(command.m_commandBuffer.Key);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current) {
    return;
  }
  auto& recording = *buffer->second.Current;
  if (recording.Closed || recording.QueryCount >= QUERY_CAPACITY) {
    return;
  }
  DeviceOperation operation(recording.Device);
  if (!operation) {
    return;
  }
  const uint32_t query = recording.QueryCount++;
  recording.Device->Dispatch.vkCmdWriteTimestamp(
      buffer->second.Handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, recording.QueryPool, query);
  if (recording.PendingWork.has_value()) {
    recording.PendingWork->End = query;
    recording.Segments.push_back(*recording.PendingWork);
    recording.PendingWork.reset();
  }
  recording.EndQuery = query;
  recording.Closed = true;
}

void PerformanceTracker::OnEndCommandBufferPost(vkEndCommandBufferCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(command.m_commandBuffer.Key);
  if (buffer != m_CommandBuffers.end() && buffer->second.Current) {
    buffer->second.Current->Closed = false;
  }
}

void PerformanceTracker::OnBindPipeline(vkCmdBindPipelineCommand& command) {
  if (command.m_pipelineBindPoint.Value != VK_PIPELINE_BIND_POINT_GRAPHICS &&
      command.m_pipelineBindPoint.Value != VK_PIPELINE_BIND_POINT_COMPUTE) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(command.m_commandBuffer.Key);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current) {
    return;
  }
  auto& recording = *buffer->second.Current;
  if (recording.Closed) {
    return;
  }

  const GITSKey key = m_Pipelines.IsTracked(command.m_pipeline.Key) ? command.m_pipeline.Key : 0;
  if (command.m_pipelineBindPoint.Value == VK_PIPELINE_BIND_POINT_GRAPHICS) {
    recording.GraphicsPipeline = key;
  } else {
    recording.ComputePipeline = key;
  }
}

void PerformanceTracker::OnWorkPre(GITSKey commandBufferKey, VkPipelineBindPoint bindPoint) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current) {
    return;
  }
  auto& recording = *buffer->second.Current;
  if (!recording.SingleView) {
    recording.CompleteCoverage = false;
    return;
  }
  const GITSKey pipeline = bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS ? recording.GraphicsPipeline
                                                                        : recording.ComputePipeline;
  if (recording.Closed || recording.PendingWork.has_value() || pipeline == 0 ||
      recording.Segments.size() >= MAX_SEGMENTS || recording.QueryCount > QUERY_CAPACITY - 3) {
    recording.CompleteCoverage = false;
    return;
  }
  DeviceOperation operation(recording.Device);
  if (!operation) {
    recording.CompleteCoverage = false;
    return;
  }
  const uint32_t query = recording.QueryCount++;
  recording.Device->Dispatch.vkCmdWriteTimestamp(
      buffer->second.Handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, recording.QueryPool, query);
  recording.PendingWork = Segment{pipeline, query, query};
}

void PerformanceTracker::OnWorkPost(GITSKey commandBufferKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current ||
      !buffer->second.Current->PendingWork.has_value()) {
    return;
  }
  auto& recording = *buffer->second.Current;
  if (recording.QueryCount >= QUERY_CAPACITY - 1) {
    recording.CompleteCoverage = false;
    recording.PendingWork.reset();
    return;
  }
  DeviceOperation operation(recording.Device);
  if (!operation) {
    recording.CompleteCoverage = false;
    recording.PendingWork.reset();
    return;
  }
  const uint32_t query = recording.QueryCount++;
  recording.Device->Dispatch.vkCmdWriteTimestamp(
      buffer->second.Handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, recording.QueryPool, query);
  recording.PendingWork->End = query;
  recording.Segments.push_back(*recording.PendingWork);
  recording.PendingWork.reset();
}

void PerformanceTracker::TrackRenderPass(GITSKey deviceKey,
                                         VkRenderPass renderPass,
                                         const VkRenderPassCreateInfo& info) {
  std::vector<uint32_t> masks(info.subpassCount, 0);
  for (auto* node = static_cast<const VkBaseInStructure*>(info.pNext); node; node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO) {
      const auto& multiview = *reinterpret_cast<const VkRenderPassMultiviewCreateInfo*>(node);
      if (multiview.subpassCount != 0) {
        if (multiview.subpassCount != masks.size() || multiview.pViewMasks == nullptr) {
          masks.clear();
        } else {
          std::copy_n(multiview.pViewMasks, masks.size(), masks.begin());
        }
      }
    }
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ViewMasks[deviceKey][renderPass] = std::move(masks);
}

void PerformanceTracker::TrackRenderPass(GITSKey deviceKey,
                                         VkRenderPass renderPass,
                                         const VkRenderPassCreateInfo2& info) {
  std::vector<uint32_t> masks;
  for (uint32_t i = 0; info.pSubpasses != nullptr && i < info.subpassCount; ++i) {
    masks.push_back(info.pSubpasses[i].viewMask);
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ViewMasks[deviceKey][renderPass] = std::move(masks);
}

void PerformanceTracker::ForgetRenderPass(GITSKey deviceKey, VkRenderPass renderPass) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto device = m_ViewMasks.find(deviceKey);
  if (device != m_ViewMasks.end()) {
    device->second.erase(renderPass);
  }
}

void PerformanceTracker::BeginRenderPass(GITSKey commandBufferKey, VkRenderPass renderPass) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current) {
    return;
  }
  auto& recording = *buffer->second.Current;
  recording.ViewMasks.clear();
  recording.Subpass = 0;
  const auto device = m_ViewMasks.find(buffer->second.DeviceKey);
  if (device != m_ViewMasks.end()) {
    const auto pass = device->second.find(renderPass);
    if (pass != device->second.end()) {
      recording.ViewMasks = pass->second;
    }
  }
  recording.SingleView = !recording.ViewMasks.empty() && recording.ViewMasks.front() == 0;
  recording.CompleteCoverage &= recording.SingleView;
}

void PerformanceTracker::NextSubpass(GITSKey commandBufferKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer == m_CommandBuffers.end() || !buffer->second.Current) {
    return;
  }
  auto& recording = *buffer->second.Current;
  ++recording.Subpass;
  recording.SingleView =
      recording.Subpass < recording.ViewMasks.size() && recording.ViewMasks[recording.Subpass] == 0;
  recording.CompleteCoverage &= recording.SingleView;
}

void PerformanceTracker::BeginRendering(GITSKey commandBufferKey, uint32_t viewMask) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer != m_CommandBuffers.end() && buffer->second.Current) {
    auto& recording = *buffer->second.Current;
    recording.SingleView = viewMask == 0;
    recording.CompleteCoverage &= recording.SingleView;
  }
}

void PerformanceTracker::EndRendering(GITSKey commandBufferKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer != m_CommandBuffers.end() && buffer->second.Current) {
    buffer->second.Current->SingleView = true;
    buffer->second.Current->ViewMasks.clear();
  }
}

void PerformanceTracker::OnExecuteCommands(vkCmdExecuteCommandsCommand& command) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto primary = m_CommandBuffers.find(command.m_commandBuffer.Key);
  if (primary == m_CommandBuffers.end() || !primary->second.Current ||
      primary->second.Level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    return;
  }
  auto& recording = *primary->second.Current;
  recording.GraphicsPipeline = 0;
  recording.ComputePipeline = 0;
  auto& secondaries = recording.Secondaries;
  for (uint32_t i = 0;
       command.m_pCommandBuffers.Value != nullptr && i < command.m_pCommandBuffers.Size; ++i) {
    const auto key = m_CommandBufferKeys.find(command.m_pCommandBuffers.Value[i]);
    if (key == m_CommandBufferKeys.end()) {
      recording.CompleteCoverage = false;
      continue;
    }
    const auto secondary = m_CommandBuffers.find(key->second);
    if (secondary == m_CommandBuffers.end() || !secondary->second.Current ||
        !secondary->second.Current->Closed ||
        secondary->second.Level != VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      recording.CompleteCoverage = false;
      continue;
    }
    if (std::find(secondaries.begin(), secondaries.end(), secondary->second.Current) ==
        secondaries.end()) {
      secondaries.push_back(secondary->second.Current);
    }
  }
}

void PerformanceTracker::OnQueueSubmit(vkQueueSubmitCommand& command) {
  std::vector<VkCommandBuffer> commandBuffers;
  for (uint32_t submit = 0; command.m_pSubmits.Value != nullptr && submit < command.m_pSubmits.Size;
       ++submit) {
    const auto& info = command.m_pSubmits.Value[submit];
    if (info.pCommandBuffers != nullptr) {
      commandBuffers.insert(commandBuffers.end(), info.pCommandBuffers,
                            info.pCommandBuffers + info.commandBufferCount);
    }
  }
  Submit(command.m_queue.Value, commandBuffers);
}

void PerformanceTracker::OnQueueSubmit2(vkQueueSubmit2Command& command) {
  std::vector<VkCommandBuffer> commandBuffers;
  for (uint32_t submit = 0; command.m_pSubmits.Value != nullptr && submit < command.m_pSubmits.Size;
       ++submit) {
    const auto& info = command.m_pSubmits.Value[submit];
    for (uint32_t i = 0; info.pCommandBufferInfos != nullptr && i < info.commandBufferInfoCount;
         ++i) {
      commandBuffers.push_back(info.pCommandBufferInfos[i].commandBuffer);
    }
  }
  Submit(command.m_queue.Value, commandBuffers);
}

void PerformanceTracker::OnQueueSubmit2(vkQueueSubmit2KHRCommand& command) {
  std::vector<VkCommandBuffer> commandBuffers;
  for (uint32_t submit = 0; command.m_pSubmits.Value != nullptr && submit < command.m_pSubmits.Size;
       ++submit) {
    const auto& info = command.m_pSubmits.Value[submit];
    for (uint32_t i = 0; info.pCommandBufferInfos != nullptr && i < info.commandBufferInfoCount;
         ++i) {
      commandBuffers.push_back(info.pCommandBufferInfos[i].commandBuffer);
    }
  }
  Submit(command.m_queue.Value, commandBuffers);
}

void PerformanceTracker::OnPresent() {
  std::vector<Frame> complete;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_Collecting.load(std::memory_order_relaxed)) {
      return;
    }
    const int64_t now = NowNs();
    if (m_NeedBaseline) {
      m_NeedBaseline = false;
      m_LastPresentNs = now;
      m_Frames.push_back(Frame{++m_FrameNumber});
      m_FrameOpen = true;
      return;
    }

    if (m_FrameOpen && !m_Frames.empty()) {
      Frame& frame = m_Frames.back();
      frame.Closed = true;
      frame.DurationNs = static_cast<size_t>(std::max<int64_t>(now - m_LastPresentNs, 0));
    }
    m_LastPresentNs = now;
    m_Frames.push_back(Frame{++m_FrameNumber});
    m_FrameOpen = true;

    while (m_Frames.size() > MAX_PENDING_FRAMES) {
      LOG_WARNING << "Rtsr - dropping a Vulkan timing frame whose GPU work did "
                     "not retire in time";
      m_Frames.pop_front();
    }
    TakeCompleteFrames(complete);
  }
  Send(std::move(complete));
}

void PerformanceTracker::PrepareDeviceDestroy(GITSKey deviceKey) {
  auto device = m_Contexts.Device(deviceKey);
  DeviceOperation operation(device);
  if (operation && device->Dispatch.vkDeviceWaitIdle != nullptr) {
    device->Dispatch.vkDeviceWaitIdle(device->Handle);
  }
}

void PerformanceTracker::OnDestroyDevice(GITSKey deviceKey) {
  std::vector<std::shared_ptr<Recording>> retired;
  std::vector<VkFence> unresolvedFences;
  std::shared_ptr<RtsrDeviceContext> device;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto buffer = m_CommandBuffers.begin(); buffer != m_CommandBuffers.end();) {
      if (buffer->second.DeviceKey == deviceKey) {
        if (buffer->second.Current) {
          device = buffer->second.Current->Device;
          retired.push_back(std::move(buffer->second.Current));
        }
        m_CommandBufferKeys.erase(buffer->second.Handle);
        buffer = m_CommandBuffers.erase(buffer);
      } else {
        ++buffer;
      }
    }
    m_CommandPools.erase(deviceKey);
    m_ViewMasks.erase(deviceKey);
    for (auto queue = m_Queues.begin(); queue != m_Queues.end();) {
      if (queue->second.Device->Key == deviceKey) {
        device = queue->second.Device;
        queue = m_Queues.erase(queue);
      } else {
        ++queue;
      }
    }
    const auto fences = m_UnresolvedFences.find(deviceKey);
    if (fences != m_UnresolvedFences.end()) {
      unresolvedFences = std::move(fences->second);
      m_UnresolvedFences.erase(fences);
    }
    const auto unresolved = m_UnresolvedRecordings.find(deviceKey);
    if (unresolved != m_UnresolvedRecordings.end()) {
      for (const auto& recording : unresolved->second) {
        if (std::find(retired.begin(), retired.end(), recording) == retired.end()) {
          retired.push_back(recording);
        }
      }
      m_UnresolvedRecordings.erase(unresolved);
    }
  }
  if (device && device->Dispatch.vkDestroyFence != nullptr) {
    for (const VkFence fence : unresolvedFences) {
      device->Dispatch.vkDestroyFence(device->Handle, fence, nullptr);
    }
  }
  for (const auto& recording : retired) {
    RetireRecording(recording, true);
  }
}

void PerformanceTracker::Submit(VkQueue queue, const std::vector<VkCommandBuffer>& commandBuffers) {
  if (commandBuffers.empty()) {
    return;
  }
  std::vector<std::shared_ptr<Recording>> recordings;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto queueInfo = m_Queues.find(queue);
    if (queueInfo == m_Queues.end()) {
      return;
    }
    for (const VkCommandBuffer commandBuffer : commandBuffers) {
      const auto key = m_CommandBufferKeys.find(commandBuffer);
      if (key == m_CommandBufferKeys.end()) {
        continue;
      }
      const auto buffer = m_CommandBuffers.find(key->second);
      if (buffer == m_CommandBuffers.end() || !buffer->second.Current ||
          !buffer->second.Current->Closed ||
          buffer->second.QueueFamily != queueInfo->second.Family) {
        continue;
      }
      CollectRecording(buffer->second.Current, recordings);
    }
    if (!recordings.empty()) {
      m_Submissions.emplace(queue, std::move(recordings));
    }
  }
}

void PerformanceTracker::OnQueueSubmitPost(VkQueue queue, VkResult result) {
  std::vector<std::shared_ptr<Recording>> recordings;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto submission = m_Submissions.find(queue);
    if (submission == m_Submissions.end()) {
      return;
    }
    recordings = std::move(submission->second);
    m_Submissions.erase(submission);
  }
  if (result != VK_SUCCESS) {
    for (const auto& recording : recordings) {
      {
        std::lock_guard<std::mutex> lock(recording->LifetimeMutex);
        recording->Submitted.store(false);
        recording->PendingReadbacks.fetch_sub(1);
      }
      if (recording->Retired.load()) {
        DestroyRecording(recording, false, false);
      }
    }
    return;
  }
  if (!recordings.empty()) {
    SubmitRecordings(queue, std::move(recordings));
  }
}

void PerformanceTracker::SubmitRecordings(VkQueue queue,
                                          std::vector<std::shared_ptr<Recording>> recordings) {
  const auto retain = [this, &recordings]() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& recording : recordings) {
      std::lock_guard<std::mutex> lifetimeLock(recording->LifetimeMutex);
      recording->Unresolved.store(true);
      recording->PendingReadbacks.fetch_sub(1);
      m_UnresolvedRecordings[recording->Device->Key].push_back(recording);
    }
    if (m_FrameOpen && !m_Frames.empty()) {
      m_Frames.back().Dropped = true;
    }
    m_Collecting.store(false);
    m_NeedBaseline = true;
  };
  const Queue queueInfo{recordings.front()->Device, recordings.front()->QueueFamily};
  if (queueInfo.Device->Dispatch.vkCreateFence == nullptr ||
      queueInfo.Device->Dispatch.vkDestroyFence == nullptr ||
      queueInfo.Device->Dispatch.vkWaitForFences == nullptr ||
      queueInfo.Device->Dispatch.vkQueueSubmit == nullptr) {
    retain();
    return;
  }
  if (!queueInfo.Device->BeginOperation()) {
    retain();
    return;
  }

  VkFenceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence = VK_NULL_HANDLE;
  if (queueInfo.Device->Dispatch.vkCreateFence(queueInfo.Device->Handle, &createInfo, nullptr,
                                               &fence) != VK_SUCCESS ||
      fence == VK_NULL_HANDLE) {
    retain();
    queueInfo.Device->EndOperation();
    return;
  }
  const VkResult submitted = queueInfo.Device->Dispatch.vkQueueSubmit(queue, 0, nullptr, fence);
  if (submitted != VK_SUCCESS) {
    retain();
    queueInfo.Device->Dispatch.vkDestroyFence(queueInfo.Device->Handle, fence, nullptr);
    queueInfo.Device->EndOperation();
    return;
  }

  size_t frameNumber = 0;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Collecting.load(std::memory_order_relaxed) && m_FrameOpen && !m_Frames.empty()) {
      frameNumber = m_Frames.back().Number;
      m_Frames.back().Pending += recordings.size();
    }
  }
  {
    std::lock_guard<std::mutex> lock(m_WorkMutex);
    m_Readbacks.push_back(Readback{queueInfo.Device, fence, frameNumber, std::move(recordings)});
  }
  m_WorkAvailable.notify_one();
}

void PerformanceTracker::CollectRecording(
    const std::shared_ptr<Recording>& recording,
    std::vector<std::shared_ptr<Recording>>& recordings) const {
  if (!recording ||
      std::find(recordings.begin(), recordings.end(), recording) != recordings.end()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(recording->LifetimeMutex);
    if (recording->Retired.load() || recording->Destroyed.load() ||
        recording->Submitted.exchange(true)) {
      return;
    }
    recording->PendingReadbacks.fetch_add(1);
  }
  recordings.push_back(recording);
  for (const auto& secondary : recording->Secondaries) {
    CollectRecording(secondary, recordings);
  }
}

void PerformanceTracker::RetireRecording(const std::shared_ptr<Recording>& recording,
                                         bool forceDestroy) {
  if (!recording) {
    return;
  }
  const bool first = !recording->Retired.exchange(true);
  if (first) {
    m_LiveRecordings.fetch_sub(1, std::memory_order_relaxed);
  }
  if (forceDestroy || recording->PendingReadbacks.load(std::memory_order_relaxed) == 0) {
    DestroyRecording(recording, false, forceDestroy);
  }
}

void PerformanceTracker::DestroyRecording(const std::shared_ptr<Recording>& recording,
                                          bool operationHeld,
                                          bool forceDestroy) {
  if (!recording) {
    return;
  }
  std::lock_guard<std::mutex> lock(recording->LifetimeMutex);
  if (recording->QueryPool == VK_NULL_HANDLE ||
      (!forceDestroy && (recording->PendingReadbacks.load(std::memory_order_relaxed) != 0 ||
                         recording->Unresolved.load(std::memory_order_relaxed)))) {
    return;
  }
  bool expected = false;
  if (!recording->Destroyed.compare_exchange_strong(expected, true)) {
    return;
  }

  bool acquired = false;
  if (!operationHeld && !forceDestroy) {
    acquired = recording->Device->BeginOperation();
    if (!acquired) {
      // The device is retiring and will release all child objects itself.
      return;
    }
  }
  if (recording->Device->Dispatch.vkDestroyQueryPool != nullptr) {
    recording->Device->Dispatch.vkDestroyQueryPool(recording->Device->Handle, recording->QueryPool,
                                                   nullptr);
  }
  if (acquired) {
    recording->Device->EndOperation();
  }
}

void PerformanceTracker::ForgetCommandBufferLocked(
    GITSKey key, std::vector<std::shared_ptr<Recording>>& retired) {
  const auto buffer = m_CommandBuffers.find(key);
  if (buffer == m_CommandBuffers.end()) {
    return;
  }
  if (buffer->second.Current) {
    retired.push_back(std::move(buffer->second.Current));
  }
  m_CommandBufferKeys.erase(buffer->second.Handle);
  m_CommandBuffers.erase(buffer);
}

int64_t PerformanceTracker::NowNs() const {
  if (m_PlaybackClock != nullptr) {
    return m_PlaybackClock->Get();
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() -
                                                              m_WallClockOrigin)
      .count();
}

void PerformanceTracker::Worker() {
  for (;;) {
    Readback readback;
    {
      std::unique_lock<std::mutex> lock(m_WorkMutex);
      m_WorkAvailable.wait(lock, [this] { return m_Stopping || !m_Readbacks.empty(); });
      if (m_Readbacks.empty()) {
        if (m_Stopping) {
          return;
        }
        continue;
      }
      readback = std::move(m_Readbacks.front());
      m_Readbacks.pop_front();
    }
    ReadBack(std::move(readback));
  }
}

void PerformanceTracker::ReadBack(Readback readback) {
  constexpr uint64_t TIMEOUT_NS = 5'000'000'000ull;
  VkResult waitResult = VK_TIMEOUT;
  if (readback.Device->Dispatch.vkWaitForFences != nullptr) {
    waitResult = readback.Device->Dispatch.vkWaitForFences(readback.Device->Handle, 1,
                                                           &readback.Fence, VK_TRUE, TIMEOUT_NS);
  }
  const bool completed = waitResult == VK_SUCCESS || waitResult == VK_ERROR_DEVICE_LOST;
  bool readbackFailed = waitResult != VK_SUCCESS;

  std::vector<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> commandBuffers;
  if (waitResult == VK_SUCCESS && readback.FrameNumber != 0) {
    for (const auto& recording : readback.Recordings) {
      auto summary = Summarize(*recording);
      if (!summary.has_value()) {
        readbackFailed = true;
      } else if (!summary->PipelineStats.empty()) {
        commandBuffers.push_back(std::move(*summary));
      }
    }
  }

  if (completed && readback.Device->Dispatch.vkDestroyFence != nullptr) {
    readback.Device->Dispatch.vkDestroyFence(readback.Device->Handle, readback.Fence, nullptr);
  } else {
    LOG_WARNING << "Rtsr - Vulkan timing fence was unresolved (VkResult " << waitResult
                << "); dropping its frame";
  }

  for (const auto& recording : readback.Recordings) {
    size_t pending{};
    {
      std::lock_guard<std::mutex> lock(recording->LifetimeMutex);
      if (!completed) {
        recording->Unresolved.store(true);
      }
      pending = recording->PendingReadbacks.fetch_sub(1) - 1;
    }
    if (completed && pending == 0 && recording->Retired.load()) {
      DestroyRecording(recording, true, waitResult == VK_ERROR_DEVICE_LOST);
    }
  }

  std::vector<Frame> complete;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!completed) {
      m_UnresolvedFences[readback.Device->Key].push_back(readback.Fence);
      auto& unresolved = m_UnresolvedRecordings[readback.Device->Key];
      for (const auto& recording : readback.Recordings) {
        if (std::find(unresolved.begin(), unresolved.end(), recording) == unresolved.end()) {
          unresolved.push_back(recording);
        }
      }
      m_Collecting.store(false);
      m_NeedBaseline = false;
      if (m_FrameOpen && !m_Frames.empty()) {
        m_Frames.pop_back();
        m_FrameOpen = false;
      }
    }
    for (Frame& frame : m_Frames) {
      if (frame.Number != readback.FrameNumber) {
        continue;
      }
      frame.Dropped = frame.Dropped || readbackFailed;
      frame.Pending -= std::min(frame.Pending, readback.Recordings.size());
      for (auto& commandBuffer : commandBuffers) {
        frame.CommandBuffers.push_back(std::move(commandBuffer));
      }
      break;
    }
    TakeCompleteFrames(complete);
  }

  readback.Device->EndOperation();
  Send(std::move(complete));
}

std::optional<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> PerformanceTracker::
    Summarize(const Recording& recording) const {
  if (!recording.CompleteCoverage || recording.QueryPool == VK_NULL_HANDLE ||
      recording.QueryCount < 2 || recording.EndQuery >= recording.QueryCount ||
      recording.QueueFamily >= recording.Device->TimestampValidBits.size() ||
      recording.Device->Dispatch.vkGetQueryPoolResults == nullptr) {
    return std::nullopt;
  }
  std::vector<uint64_t> timestamps(recording.QueryCount);
  const VkResult result = recording.Device->Dispatch.vkGetQueryPoolResults(
      recording.Device->Handle, recording.QueryPool, 0, recording.QueryCount,
      timestamps.size() * sizeof(uint64_t), timestamps.data(), sizeof(uint64_t),
      VK_QUERY_RESULT_64_BIT);
  if (result != VK_SUCCESS) {
    return std::nullopt;
  }

  const uint32_t validBits = recording.Device->TimestampValidBits[recording.QueueFamily];
  rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO summary;
  summary.Begin = 0;
  summary.Duration =
      Nanoseconds(TimestampDelta(timestamps[0], timestamps[recording.EndQuery], validBits),
                  recording.Device->TimestampPeriod);
  summary.PipelineStats.reserve(recording.Segments.size());
  for (const Segment& segment : recording.Segments) {
    if (segment.Begin >= timestamps.size() || segment.End >= timestamps.size()) {
      continue;
    }
    rtsr::RTSR_PIPELINE_TIME_STATS_INFO pipeline;
    pipeline.Id = segment.PipelineKey;
    pipeline.Begin = 0;
    pipeline.Duration =
        Nanoseconds(TimestampDelta(timestamps[segment.Begin], timestamps[segment.End], validBits),
                    recording.Device->TimestampPeriod);
    summary.PipelineStats.push_back(std::move(pipeline));
  }
  return summary;
}

void PerformanceTracker::TakeCompleteFrames(std::vector<Frame>& complete) {
  while (!m_Frames.empty()) {
    Frame& frame = m_Frames.front();
    if (!frame.Closed || frame.Pending != 0) {
      break;
    }
    if (!frame.Dropped) {
      complete.push_back(std::move(frame));
    }
    m_Frames.pop_front();
  }
}

void PerformanceTracker::Send(std::vector<Frame> frames) {
  for (Frame& frame : frames) {
    rtsr::RTSR_REPORT_TIME_STATS_MESSAGE message;
    message.FrameNumber = frame.Number;
    message.Begin = 0;
    message.Duration = frame.DurationNs;
    message.CommandBufferStats = std::move(frame.CommandBuffers);
    m_Client.ReportTimeStats(std::move(message));
  }
}

} // namespace gits::vulkan
