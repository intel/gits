// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcesDump.h"
#include "imageDumpWriter.h"
#include "formatBlockInfo.h"
#include "dispatchTableAuto.h"
#include "enumToStrAuto.h"
#include "log.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <string>

namespace gits {
namespace vulkan {

namespace {

std::string AspectSuffix(const FormatBlockInfo& info, VkImageAspectFlags aspect) {
  switch (aspect) {
  case VK_IMAGE_ASPECT_PLANE_0_BIT:
    return "_plane_0";
  case VK_IMAGE_ASPECT_PLANE_1_BIT:
    return "_plane_1";
  case VK_IMAGE_ASPECT_PLANE_2_BIT:
    return "_plane_2";
  case VK_IMAGE_ASPECT_DEPTH_BIT:
    return info.StencilBytes > 0 ? "_depth" : "";
  case VK_IMAGE_ASPECT_STENCIL_BIT:
    return info.DepthBytes > 0 ? "_stencil" : "";
  default:
    return "";
  }
}

} // namespace

ExecutionFence::~ExecutionFence() {
  m_DispatchTable.vkDestroyFence(m_Device, m_Fence, nullptr);
}

VkResult ExecutionFence::Wait(uint64_t timeout) const {
  return m_DispatchTable.vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, timeout);
}

DumpBatch::~DumpBatch() {
  for (StagedResource& resource : Resources) {
    if (resource.Staging) {
      m_DispatchTable.vkDestroyBuffer(m_Device, resource.Staging, nullptr);
    }
    if (resource.Memory) {
      m_DispatchTable.vkFreeMemory(m_Device, resource.Memory, nullptr);
    }
  }
}

bool DumpBatch::Claim() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return !std::exchange(m_Claimed, true);
}

void DumpBatch::MarkWritten(bool executed) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Written = true;
  m_Executed = executed;
  m_WrittenCV.notify_all();
}

void DumpBatch::WaitWritten() {
  std::unique_lock<std::mutex> lock(m_Mutex);
  m_WrittenCV.wait(lock, [this] { return m_Written; });
}

bool DumpBatch::IsWritten() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Written;
}

bool DumpBatch::IsExecuted() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Written && m_Executed;
}

ResourcesDump::ResourcesDump(VkDevice device,
                             VkDeviceLevelDispatchTable& dispatchTable,
                             const VkPhysicalDeviceMemoryProperties& memoryProperties,
                             bool separateDepthStencilLayouts)
    : m_Device(device),
      m_DispatchTable(dispatchTable),
      m_MemoryProperties(memoryProperties),
      m_SeparateDepthStencilLayouts(separateDepthStencilLayouts) {
  m_Worker = std::thread(&ResourcesDump::WorkerThread, this);
}

ResourcesDump::~ResourcesDump() {
  Finish(std::chrono::milliseconds::zero());
  FreeWrittenCopies();
  // The work of the copies left is not known to have completed: their fence was not submitted,
  // waiting for it failed, or the worker stopped before it signaled. The device may still use their
  // resources, and waiting for it may never return, so they are never released.
  if (!m_AbandonedJobs.empty() || !m_SubmittedCopies.empty() || !m_UntrackedBatches.empty() ||
      !m_FailedJobs.empty()) {
    struct Retained {
      std::vector<Job> AbandonedJobs;
      std::vector<SubmittedCopies> Copies;
      std::vector<std::shared_ptr<DumpBatch>> UntrackedBatches;
      std::vector<Job> FailedJobs;
    };
    static_cast<void>(new Retained{std::move(m_AbandonedJobs), std::move(m_SubmittedCopies),
                                   std::move(m_UntrackedBatches), std::move(m_FailedJobs)});
    return;
  }
  for (auto& [queueFamilyIndex, commandPool] : m_CommandPools) {
    m_DispatchTable.vkDestroyCommandPool(m_Device, commandPool, nullptr);
  }
}

std::shared_ptr<DumpBatch> ResourcesDump::RecordDumps(VkCommandBuffer commandBuffer,
                                                      std::vector<DumpTarget> targets) {
  auto batch = std::make_shared<DumpBatch>(m_Device, m_DispatchTable);
  for (DumpTarget& target : targets) {
    StagedResource resource;
    resource.Target = std::move(target);
    if (resource.Target.Buffer) {
      resource.Size = resource.Target.BufferSize;
    } else {
      resource.Size = CopyRegions(resource.Target, resource.Regions);
      if (resource.Regions.empty()) {
        continue;
      }
    }
    CreateStaging(resource);
    batch->Resources.push_back(std::move(resource));
  }
  if (!batch->Resources.empty()) {
    RecordCopies(commandBuffer, *batch);
  }
  return batch;
}

void ResourcesDump::RecordCopies(VkCommandBuffer commandBuffer, DumpBatch& batch) {
  std::vector<VkImageMemoryBarrier> toTransfer;
  for (const StagedResource& resource : batch.Resources) {
    if (resource.Target.Image) {
      const std::vector<VkImageMemoryBarrier> barriers =
          TransferSrcBarriers(resource.Target, m_SeparateDepthStencilLayouts);
      toTransfer.insert(toTransfer.end(), barriers.begin(), barriers.end());
    }
  }
  std::vector<VkImageMemoryBarrier> back = toTransfer;
  for (VkImageMemoryBarrier& barrier : back) {
    std::swap(barrier.oldLayout, barrier.newLayout);
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  }

  // ALL_COMMANDS in the first scope chains the copies to the semaphore waits of the submission,
  // whatever their stages, as well as to the earlier work of the queue.
  VkMemoryBarrier beforeCopies{};
  beforeCopies.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  beforeCopies.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  beforeCopies.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  m_DispatchTable.vkCmdPipelineBarrier(
      commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
      &beforeCopies, 0, nullptr, static_cast<uint32_t>(toTransfer.size()), toTransfer.data());

  for (const StagedResource& resource : batch.Resources) {
    if (resource.Target.Buffer) {
      const VkBufferCopy copy{0, 0, resource.Size};
      m_DispatchTable.vkCmdCopyBuffer(commandBuffer, resource.Target.Buffer, resource.Staging, 1,
                                      &copy);
    } else {
      for (const VkImageLayout layout :
           {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}) {
        std::vector<VkBufferImageCopy> regions;
        for (const VkBufferImageCopy& region : resource.Regions) {
          if (CopySourceLayout(resource.Target, region.imageSubresource) == layout) {
            regions.push_back(region);
          }
        }
        if (!regions.empty()) {
          m_DispatchTable.vkCmdCopyImageToBuffer(
              commandBuffer, resource.Target.Image, layout, resource.Staging,
              static_cast<uint32_t>(regions.size()), regions.data());
        }
      }
    }
  }

  // Later commands of the application wait for the copies before writing the resources.
  VkMemoryBarrier afterCopies{};
  afterCopies.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  afterCopies.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  afterCopies.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  m_DispatchTable.vkCmdPipelineBarrier(
      commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &afterCopies, 0,
      nullptr, static_cast<uint32_t>(back.size()), back.data());
}

uint32_t ResourcesDump::FindStagingMemoryType(uint32_t memoryTypeBits, bool& coherent) const {
  constexpr VkMemoryPropertyFlags preferences[] = {
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
  };
  for (VkMemoryPropertyFlags required : preferences) {
    for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; ++i) {
      const VkMemoryPropertyFlags flags = m_MemoryProperties.memoryTypes[i].propertyFlags;
      if ((memoryTypeBits & (1u << i)) && (flags & required) == required) {
        coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        return i;
      }
    }
  }
  return UINT32_MAX;
}

void ResourcesDump::CreateStaging(StagedResource& resource) {
  const std::string what = resource.Target.Name.filename().string();
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = resource.Size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkResult result =
      m_DispatchTable.vkCreateBuffer(m_Device, &bufferInfo, nullptr, &resource.Staging);
  if (result != VK_SUCCESS) {
    resource.Staging = VK_NULL_HANDLE;
    throw std::runtime_error("ResourcesDump: cannot create a staging buffer of " +
                             std::to_string(resource.Size) + " bytes for " + what + " (" +
                             toStr(result) + ")");
  }

  VkMemoryRequirements requirements{};
  m_DispatchTable.vkGetBufferMemoryRequirements(m_Device, resource.Staging, &requirements);
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex =
      FindStagingMemoryType(requirements.memoryTypeBits, resource.Coherent);
  result =
      allocateInfo.memoryTypeIndex == UINT32_MAX
          ? VK_ERROR_OUT_OF_HOST_MEMORY
          : m_DispatchTable.vkAllocateMemory(m_Device, &allocateInfo, nullptr, &resource.Memory);
  if (result == VK_SUCCESS) {
    result = m_DispatchTable.vkBindBufferMemory(m_Device, resource.Staging, resource.Memory, 0);
  }
  if (result != VK_SUCCESS) {
    m_DispatchTable.vkDestroyBuffer(m_Device, resource.Staging, nullptr);
    resource.Staging = VK_NULL_HANDLE;
    if (resource.Memory) {
      m_DispatchTable.vkFreeMemory(m_Device, resource.Memory, nullptr);
      resource.Memory = VK_NULL_HANDLE;
    }
    throw std::runtime_error("ResourcesDump: cannot allocate " + std::to_string(requirements.size) +
                             " bytes of host visible memory for " + what + " (" + toStr(result) +
                             "). Dump fewer resources at once or dump on a system with more "
                             "memory.");
  }
}

std::shared_ptr<ExecutionFence> ResourcesDump::CreateFence() {
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence{};
  const VkResult result = m_DispatchTable.vkCreateFence(m_Device, &fenceInfo, nullptr, &fence);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("ResourcesDump: cannot create a fence (" + toStr(result) + ")");
  }
  return std::make_shared<ExecutionFence>(m_Device, m_DispatchTable, fence);
}

void ResourcesDump::SubmitFence(VkQueue queue, const ExecutionFence& fence, const char* after) {
  const VkResult result = m_DispatchTable.vkQueueSubmit(queue, 0, nullptr, fence.Handle());
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::string("ResourcesDump: cannot submit a fence after the ") +
                             after + " (" + toStr(result) + ")");
  }
}

void ResourcesDump::WriteAfterExecution(VkQueue queue,
                                        std::vector<std::shared_ptr<DumpBatch>> batches,
                                        std::vector<std::shared_ptr<DumpBatch>> executedAgain) {
  if (batches.empty() && executedAgain.empty()) {
    return;
  }
  std::shared_ptr<ExecutionFence> fence;
  try {
    fence = CreateFence();
    SubmitFence(queue, *fence, "dumped commands");
  } catch (...) {
    m_UntrackedBatches.insert(m_UntrackedBatches.end(), batches.begin(), batches.end());
    m_UntrackedBatches.insert(m_UntrackedBatches.end(), executedAgain.begin(), executedAgain.end());
    throw;
  }
  for (const auto& batch : executedAgain) {
    batch->Execution = fence;
  }
  for (const auto& batch : batches) {
    batch->Submitted = true;
    batch->Execution = fence;
  }
  // The worker keeps the fence until it signals, even if no batch is written after it.
  Enqueue({std::move(fence), std::move(batches), std::move(executedAgain)});
}

VkCommandBuffer ResourcesDump::BeginCopies(uint32_t queueFamilyIndex) {
  VkCommandPool& commandPool = m_CommandPools[queueFamilyIndex];
  if (!commandPool) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    const VkResult result =
        m_DispatchTable.vkCreateCommandPool(m_Device, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
      m_CommandPools.erase(queueFamilyIndex);
      throw std::runtime_error("ResourcesDump: cannot create a command pool (" + toStr(result) +
                               ")");
    }
  }

  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  VkCommandBuffer commandBuffer{};
  VkResult result =
      m_DispatchTable.vkAllocateCommandBuffers(m_Device, &allocateInfo, &commandBuffer);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("ResourcesDump: cannot allocate a command buffer (" + toStr(result) +
                             ")");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  result = m_DispatchTable.vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    m_DispatchTable.vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
    throw std::runtime_error("ResourcesDump: cannot begin a command buffer (" + toStr(result) +
                             ")");
  }
  return commandBuffer;
}

VkResult ResourcesDump::SubmitWithSignals(VkQueue queue,
                                          VkCommandBuffer commandBuffer,
                                          const SubmitSignals& signals) {
  // The copies run in the first batch. The signals of each original batch stay in a batch of their
  // own, and the first synchronization scope of every signal includes the copies, as they come
  // earlier in submission order.
  const size_t batchCount = std::max<size_t>(1, signals.Batches.size());
  if (signals.SubmittedWith == SubmitSignals::Function::QueueSubmit) {
    std::vector<std::vector<VkSemaphore>> semaphores(batchCount);
    std::vector<std::vector<uint64_t>> values(batchCount);
    std::vector<VkTimelineSemaphoreSubmitInfo> timelineInfos(batchCount);
    std::vector<VkSubmitInfo> submitInfos(batchCount);
    for (size_t i = 0; i < batchCount; ++i) {
      for (size_t j = 0; i < signals.Batches.size() && j < signals.Batches[i].size(); ++j) {
        semaphores[i].push_back(signals.Batches[i][j].semaphore);
        values[i].push_back(signals.Batches[i][j].value);
      }
      VkTimelineSemaphoreSubmitInfo& timelineInfo = timelineInfos[i];
      timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
      timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(values[i].size());
      timelineInfo.pSignalSemaphoreValues = values[i].data();
      VkSubmitInfo& submitInfo = submitInfos[i];
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.pNext = signals.TimelineValues ? &timelineInfo : nullptr;
      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(semaphores[i].size());
      submitInfo.pSignalSemaphores = semaphores[i].data();
    }
    submitInfos[0].commandBufferCount = commandBuffer ? 1 : 0;
    submitInfos[0].pCommandBuffers = &commandBuffer;
    return m_DispatchTable.vkQueueSubmit(queue, static_cast<uint32_t>(batchCount),
                                         submitInfos.data(), signals.Fence);
  }
  VkCommandBufferSubmitInfo commandBufferInfo{};
  commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  commandBufferInfo.commandBuffer = commandBuffer;
  std::vector<VkSubmitInfo2> submitInfos(batchCount);
  for (size_t i = 0; i < batchCount; ++i) {
    VkSubmitInfo2& submitInfo = submitInfos[i];
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    if (i < signals.Batches.size()) {
      submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.Batches[i].size());
      submitInfo.pSignalSemaphoreInfos = signals.Batches[i].data();
    }
  }
  submitInfos[0].commandBufferInfoCount = commandBuffer ? 1 : 0;
  submitInfos[0].pCommandBufferInfos = &commandBufferInfo;
  const PFN_vkQueueSubmit2 queueSubmit2 =
      signals.SubmittedWith == SubmitSignals::Function::QueueSubmit2
          ? m_DispatchTable.vkQueueSubmit2
          : m_DispatchTable.vkQueueSubmit2KHR;
  return queueSubmit2(queue, static_cast<uint32_t>(batchCount), submitInfos.data(), signals.Fence);
}

void ResourcesDump::FreeWrittenCopies() {
  std::erase_if(m_SubmittedCopies, [this](SubmittedCopies& copies) {
    if (!copies.Batch->IsExecuted()) {
      return false;
    }
    m_DispatchTable.vkFreeCommandBuffers(m_Device, copies.CommandPool, 1, &copies.CommandBuffer);
    return true;
  });
}

void ResourcesDump::DumpAfterSubmissions(VkQueue queue,
                                         uint32_t queueFamilyIndex,
                                         std::vector<DumpTarget> targets,
                                         const SubmitSignals& signals) {
  FreeWrittenCopies();
  VkCommandBuffer commandBuffer{};
  std::shared_ptr<DumpBatch> batch;
  std::shared_ptr<ExecutionFence> fence;
  std::exception_ptr error;
  const bool signalsAnything =
      signals.Fence || std::any_of(signals.Batches.begin(), signals.Batches.end(),
                                   [](const auto& batch) { return !batch.empty(); });
  if (!targets.empty() || signalsAnything) {
    try {
      commandBuffer = BeginCopies(queueFamilyIndex);
      fence = CreateFence();
    } catch (...) {
      error = std::current_exception();
    }
  }
  if (fence) {
    try {
      batch = RecordDumps(commandBuffer, std::move(targets));
    } catch (...) {
      error = std::current_exception();
      batch = std::make_shared<DumpBatch>(m_Device, m_DispatchTable);
    }
    if (batch->Resources.empty()) {
      // Without commands before them, the signals would not wait for the semaphore waits of the
      // submission they are taken out of, as a fence of that submission does.
      m_DispatchTable.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                                           nullptr, 0, nullptr);
    }
  }
  const VkCommandPool commandPool =
      commandBuffer ? m_CommandPools.at(queueFamilyIndex) : VK_NULL_HANDLE;
  if (commandBuffer) {
    const VkResult endResult = m_DispatchTable.vkEndCommandBuffer(commandBuffer);
    if (endResult != VK_SUCCESS && !error) {
      error = std::make_exception_ptr(
          std::runtime_error("ResourcesDump: cannot record the copies of the dumped resources (" +
                             toStr(endResult) + ")"));
    }
    if (endResult != VK_SUCCESS || !fence || (batch->Resources.empty() && !signalsAnything)) {
      m_DispatchTable.vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
      commandBuffer = VK_NULL_HANDLE;
    }
  }

  // The signals are submitted even if nothing is copied, or the work waiting for them never runs.
  VkResult result = SubmitWithSignals(queue, commandBuffer, signals);
  if (result != VK_SUCCESS) {
    if (commandBuffer) {
      m_DispatchTable.vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
    }
    if (!error) {
      throw std::runtime_error("ResourcesDump: cannot submit the copies of the dumped resources (" +
                               toStr(result) + ")");
    }
    std::rethrow_exception(error);
  }
  if (commandBuffer) {
    // The worker waits for the copies, as the submitted work may wait for semaphores that later
    // replayed calls signal.
    m_SubmittedCopies.push_back({commandPool, commandBuffer, batch});
    SubmitFence(queue, *fence, "dumped resources");
    batch->Submitted = true;
    batch->Execution = fence;
    Enqueue({std::move(fence), {batch}});
  }
  if (error) {
    std::rethrow_exception(error);
  }
}

size_t ResourcesDump::Finish(std::chrono::milliseconds timeout) {
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    size_t unfinished = m_UnfinishedJobs;
    while (unfinished > 0 &&
           m_IdleCV.wait_for(lock, timeout, [&] { return m_UnfinishedJobs < unfinished; })) {
      unfinished = m_UnfinishedJobs;
    }
    m_Stop = true;
  }
  m_JobsCV.notify_all();
  if (m_Worker.joinable()) {
    m_Worker.join();
  }
  const std::vector<std::string> abandoned = std::exchange(m_AbandonedDumps, {});
  for (const std::string& name : abandoned) {
    LOG_ERROR << "ResourcesDump: " << name
              << " is not written, as the work it waits for did not complete";
  }
  return abandoned.size();
}

std::string ResourcesDump::TakeWorkerError() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return std::exchange(m_WorkerError, {});
}

void ResourcesDump::Enqueue(Job job) {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Jobs.push_back(std::move(job));
    ++m_UnfinishedJobs;
  }
  m_JobsCV.notify_one();
}

void ResourcesDump::WorkerThread() {
  // The work of a job may wait for semaphores that later replayed calls signal, which may wait for
  // the jobs of other queues to be written. So every job is written once its own fence signals,
  // whatever the order of the jobs.
  constexpr uint64_t pollTimeout = 10000000ull;
  std::vector<Job> pending;
  while (true) {
    bool stop = false;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      if (pending.empty()) {
        m_JobsCV.wait(lock, [this] { return !m_Jobs.empty() || m_Stop; });
      }
      std::move(m_Jobs.begin(), m_Jobs.end(), std::back_inserter(pending));
      m_Jobs.clear();
      stop = m_Stop;
    }

    bool written = false;
    for (auto it = pending.begin(); it != pending.end();) {
      const VkResult result = it->Fence->Wait(0);
      if (result == VK_TIMEOUT) {
        ++it;
        continue;
      }
      std::string error;
      if (result == VK_SUCCESS) {
        WriteJob(*it);
      } else {
        // The work may still be pending, so its resources are kept.
        error = "ResourcesDump: waiting for the copies of the dumped resources failed (" +
                toStr(result) + "), so they are not written";
        LOG_ERROR << error;
        for (const auto& batch : it->Batches) {
          if (batch->Claim()) {
            batch->MarkWritten(false);
          }
        }
        m_FailedJobs.push_back(std::move(*it));
      }
      it = pending.erase(it);
      written = true;
      {
        std::lock_guard<std::mutex> lock(m_Mutex);
        --m_UnfinishedJobs;
        if (m_WorkerError.empty()) {
          m_WorkerError = std::move(error);
        }
      }
      m_IdleCV.notify_all();
    }
    if (stop) {
      // The fences left may never signal, so their jobs are kept without waiting for them.
      std::vector<std::string> abandoned;
      for (const Job& job : pending) {
        for (const auto& batch : job.Batches) {
          if (batch->Claim()) {
            batch->MarkWritten(false);
            for (const StagedResource& resource : batch->Resources) {
              abandoned.push_back(resource.Target.Name.string());
            }
          }
        }
      }
      {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_UnfinishedJobs -= pending.size();
        std::move(pending.begin(), pending.end(), std::back_inserter(m_AbandonedJobs));
        std::move(abandoned.begin(), abandoned.end(), std::back_inserter(m_AbandonedDumps));
      }
      m_IdleCV.notify_all();
      return;
    }
    if (!written && !pending.empty()) {
      pending.front().Fence->Wait(pollTimeout);
    }
  }
}

void ResourcesDump::WriteJob(Job& job) {
  job.Fence.reset();
  for (const auto& batch : job.Batches) {
    if (batch->Claim()) {
      WriteBatch(*batch);
    }
  }
}

void ResourcesDump::WriteExecuted(DumpBatch& batch) {
  if (batch.Claim()) {
    WriteBatch(batch);
  } else {
    batch.WaitWritten();
  }
}

void ResourcesDump::WriteBatch(DumpBatch& batch) {
  for (StagedResource& resource : batch.Resources) {
    try {
      WriteResource(resource);
    } catch (const std::exception& e) {
      LOG_ERROR << "ResourcesDump: writing " << resource.Target.Name.string()
                << " failed: " << e.what();
    }
  }
  batch.MarkWritten(true);
}

void ResourcesDump::WriteResource(StagedResource& resource) {
  void* mapped = nullptr;
  VkResult result =
      m_DispatchTable.vkMapMemory(m_Device, resource.Memory, 0, VK_WHOLE_SIZE, 0, &mapped);
  if (result != VK_SUCCESS) {
    LOG_ERROR << "ResourcesDump: cannot map the staging memory of " << resource.Target.Name.string()
              << " (" << toStr(result) << ")";
    return;
  }
  if (!resource.Coherent) {
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = resource.Memory;
    range.size = VK_WHOLE_SIZE;
    result = m_DispatchTable.vkInvalidateMappedMemoryRanges(m_Device, 1, &range);
    if (result != VK_SUCCESS) {
      m_DispatchTable.vkUnmapMemory(m_Device, resource.Memory);
      LOG_ERROR << "ResourcesDump: cannot make the staging memory of "
                << resource.Target.Name.string() << " visible to the host (" << toStr(result)
                << "), so it is not written";
      return;
    }
  }

  const auto* data = static_cast<const uint8_t*>(mapped);
  if (resource.Target.Buffer) {
    std::filesystem::path path = resource.Target.Name;
    path += ".bin";
    if (WriteBinary(path, data, static_cast<size_t>(resource.Size))) {
      LOG_INFO << "ResourcesDump: written " << path.string();
    } else {
      LOG_ERROR << "ResourcesDump: cannot write " << path.string();
    }
  } else {
    WriteImage(resource, data);
  }
  m_DispatchTable.vkUnmapMemory(m_Device, resource.Memory);
}

void ResourcesDump::WriteImage(const StagedResource& resource, const uint8_t* data) {
  const DumpTarget& target = resource.Target;
  const FormatBlockInfo info = GetFormatBlockInfo(target.Format);
  unsigned written = 0;
  unsigned raw = 0;
  std::vector<uint8_t> rgba;
  for (const VkBufferImageCopy& region : resource.Regions) {
    const VkImageSubresourceLayers& subresource = region.imageSubresource;
    const RegionTexels texels = GetRegionTexels(info, subresource.aspectMask);
    const uint32_t width = region.imageExtent.width;
    const uint32_t height = region.imageExtent.height;
    const uint32_t depth = region.imageExtent.depth;
    const size_t rowBytes =
        static_cast<size_t>((width + texels.BlockWidth - 1) / texels.BlockWidth) *
        texels.BytesPerBlock;
    const size_t sliceBytes = rowBytes * ((height + texels.BlockHeight - 1) / texels.BlockHeight);

    std::filesystem::path name = target.Name;
    name += AspectSuffix(info, subresource.aspectMask);
    if (target.Layouts.ArrayLayers() > 1) {
      name += "_array_" + std::to_string(subresource.baseArrayLayer);
    }
    if (target.Layouts.MipLevels() > 1) {
      name += "_mip_" + std::to_string(subresource.mipLevel);
    }

    for (uint32_t slice = 0; slice < depth; ++slice) {
      std::filesystem::path sliceName = name;
      if (target.Extent.depth > 1) {
        sliceName += "_slice_" + std::to_string(region.imageOffset.z + slice);
      }
      const uint8_t* sliceData = data + region.bufferOffset + slice * sliceBytes;
      const bool converted =
          texels.BlockWidth == 1 && texels.BlockHeight == 1 &&
          ConvertToRgba8(target.Format, subresource.aspectMask, sliceData, width, height, rgba);
      bool ok = false;
      if (converted) {
        ok = !WriteRgba8Image(sliceName, target.FileFormat, width, height, rgba).empty();
      } else {
        sliceName += ".bin";
        ok = WriteBinary(sliceName, sliceData, sliceBytes);
        ++raw;
      }
      if (ok) {
        ++written;
      } else {
        LOG_ERROR << "ResourcesDump: cannot write " << sliceName.string();
      }
    }
  }
  LOG_INFO << "ResourcesDump: written " << target.Name.string() << " (" << written << " file"
           << (written == 1 ? "" : "s") << ")";
  if (raw > 0) {
    LOG_INFO << "ResourcesDump: " << target.Name.filename().string() << " has format "
             << toStr(target.Format) << ", which has no conversion to an image, so its texels "
             << "are written as they are to .bin files";
  }
}

} // namespace vulkan
} // namespace gits
