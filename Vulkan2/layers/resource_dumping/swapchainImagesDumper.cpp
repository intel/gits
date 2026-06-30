// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "swapchainImagesDumper.h"
#include "dispatchTableAuto.h"
#include "configurator.h"
#include "log.h"

#include <stb_image_write.h>
#include <algorithm>
#include <cstring>
#include <cerrno>

namespace gits {
namespace vulkan {

SwapchainImagesDumper::SwapchainImagesDumper(VkDevice device,
                                             const VkSwapchainCreateInfoKHR& swapchainCreateInfo,
                                             const VkPhysicalDeviceMemoryProperties& memProperties,
                                             VkDeviceLevelDispatchTable& dispatchTable)
    : m_DispatchTable(dispatchTable),
      m_Device(device),
      m_MemProperties(memProperties),
      m_Format(swapchainCreateInfo.imageFormat),
      m_Width(swapchainCreateInfo.imageExtent.width),
      m_Height(swapchainCreateInfo.imageExtent.height) {
  // Normally the swapchain format 32-bit RGBA/BGRA with alpha present
  if (m_Format != VK_FORMAT_R8G8B8A8_UNORM && m_Format != VK_FORMAT_R8G8B8A8_SRGB &&
      m_Format != VK_FORMAT_R8G8B8A8_SNORM && m_Format != VK_FORMAT_B8G8R8A8_UNORM &&
      m_Format != VK_FORMAT_B8G8R8A8_SRGB && m_Format != VK_FORMAT_B8G8R8A8_SNORM) {
    LOG_ERROR << "SwapchainImagesDumper: unsupported swapchain format "
              << static_cast<int>(m_Format)
              << ". Only 4-component RGBA/BGRA formats are supported for screenshot dumping.";
  }
}

void SwapchainImagesDumper::AllocateBuffers(const std::vector<uint32_t>& queueFamilyIndices) {
  // Fences and staging buffers are queue family independent
  // and can be created up front.
  for (auto& stagedFrame : m_StagedFrames) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkResult result =
        m_DispatchTable.vkCreateFence(m_Device, &fenceInfo, nullptr, &stagedFrame.Fence);
    GITS_ASSERT(result == VK_SUCCESS);

    CreateStagingBuffer(stagedFrame);
  }

  // Command pools and buffers are queue family dependent
  for (uint32_t queueFamilyIndex : queueFamilyIndices) {
    if (m_CommandPools.count(queueFamilyIndex)) {
      continue;
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result =
        m_DispatchTable.vkCreateCommandPool(m_Device, &poolInfo, nullptr, &commandPool);
    GITS_ASSERT(result == VK_SUCCESS);

    for (auto& stagedFrame : m_StagedFrames) {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = 1;

      VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
      result = m_DispatchTable.vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
      GITS_ASSERT(result == VK_SUCCESS);
      stagedFrame.CommandBuffers[queueFamilyIndex] = commandBuffer;
    }

    m_CommandPools[queueFamilyIndex] = commandPool;
  }
}

void SwapchainImagesDumper::StartWorkerThread() {
  // Create a pool of workers to encode staged frames in parallel
  unsigned int hwThreads = std::thread::hardware_concurrency();
  unsigned int workerCount = (hwThreads > 1) ? (hwThreads - 1) : 1;
  workerCount = std::min<unsigned int>(workerCount, MAX_STAGED_FRAMES);

  m_Workers.reserve(workerCount);
  for (unsigned int i = 0; i < workerCount; ++i) {
    m_Workers.emplace_back(&SwapchainImagesDumper::WorkerThread, this);
  }
}

SwapchainImagesDumper::~SwapchainImagesDumper() {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Shutdown = true;
  }
  m_FrameCopySubmittedCV.notify_all();

  for (auto& worker : m_Workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  for (auto& stagedFrame : m_StagedFrames) {
    if (stagedFrame.StagingBuffer) {
      m_DispatchTable.vkDestroyBuffer(m_Device, stagedFrame.StagingBuffer, nullptr);
    }
    if (stagedFrame.StagingMemory) {
      m_DispatchTable.vkFreeMemory(m_Device, stagedFrame.StagingMemory, nullptr);
    }
    if (stagedFrame.Fence) {
      m_DispatchTable.vkDestroyFence(m_Device, stagedFrame.Fence, nullptr);
    }
  }
  for (auto& [queueFamilyIndex, commandPool] : m_CommandPools) {
    if (commandPool) {
      m_DispatchTable.vkDestroyCommandPool(m_Device, commandPool, nullptr);
    }
  }
}

uint32_t SwapchainImagesDumper::GetMemoryTypeIndexWithFlags(
    uint32_t memoryTypeBits, VkMemoryPropertyFlags requiredFlags) const {
  for (uint32_t i = 0; i < m_MemProperties.memoryTypeCount; ++i) {
    if (!(memoryTypeBits & (1 << i))) {
      continue;
    }
    if ((m_MemProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
      return i;
    }
  }
  return UINT32_MAX;
}

void SwapchainImagesDumper::CreateStagingBuffer(StagedFrame& stagedFrame) {
  VkDeviceSize rowPitch = m_Width * BYTES_PER_PIXEL;
  VkDeviceSize bufferSize = rowPitch * m_Height;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result =
      m_DispatchTable.vkCreateBuffer(m_Device, &bufferInfo, nullptr, &stagedFrame.StagingBuffer);
  GITS_ASSERT(result == VK_SUCCESS);

  VkMemoryRequirements memRequirements;
  m_DispatchTable.vkGetBufferMemoryRequirements(m_Device, stagedFrame.StagingBuffer,
                                                &memRequirements);

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memRequirements.size;

  // Cached bit is especailly important for good performance
  constexpr VkMemoryPropertyFlags preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

  constexpr VkMemoryPropertyFlags requiredFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  uint32_t memTypeIndex =
      GetMemoryTypeIndexWithFlags(memRequirements.memoryTypeBits, preferredFlags);
  if (memTypeIndex != UINT32_MAX) {
    m_StagingMemoryIsCached = true;
  } else {
    memTypeIndex = GetMemoryTypeIndexWithFlags(memRequirements.memoryTypeBits, requiredFlags);
    m_StagingMemoryIsCached = false;
  }
  GITS_ASSERT(memTypeIndex != UINT32_MAX);

  if (memTypeIndex == UINT32_MAX) {
    LOG_ERROR << "Failed to find suitable memory type for screenshot dump staging buffer.";
    return;
  }

  memAllocInfo.memoryTypeIndex = memTypeIndex;
  result = m_DispatchTable.vkAllocateMemory(m_Device, &memAllocInfo, nullptr,
                                            &stagedFrame.StagingMemory);
  GITS_ASSERT(result == VK_SUCCESS);

  result = m_DispatchTable.vkBindBufferMemory(m_Device, stagedFrame.StagingBuffer,
                                              stagedFrame.StagingMemory, 0);
  GITS_ASSERT(result == VK_SUCCESS);
}

SwapchainImagesDumper::StagedFrame* SwapchainImagesDumper::ReserveStagedFrame() {
  bool expectedToBeReserved = false;
  for (auto& stagedFrame : m_StagedFrames) {
    expectedToBeReserved = false;
    if (stagedFrame.IsReservedForDump.compare_exchange_strong(expectedToBeReserved, true)) {
      return &stagedFrame;
    }
  }
  return nullptr;
}

bool SwapchainImagesDumper::SubmitCommandBuffer(VkQueue queue,
                                                uint32_t queueFamilyIndex,
                                                VkImage swapchainImage,
                                                const std::string& dumpName,
                                                uint32_t waitSemaphoreCount,
                                                const VkSemaphore* pWaitSemaphores) {
  StagedFrame* stagedFrame = nullptr;
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_StagedFrameFreeCV.wait(lock, [this, &stagedFrame] {
      stagedFrame = ReserveStagedFrame();
      return stagedFrame || m_Shutdown;
    });
  }
  if (m_Shutdown) {
    return false;
  }

  // The command buffer must come from a pool created for the present queue's
  // family.
  auto commandBufferIt = stagedFrame->CommandBuffers.find(queueFamilyIndex);
  if (commandBufferIt == stagedFrame->CommandBuffers.end()) {
    LOG_ERROR << "SwapchainImagesDumper: no command resources for present queue family "
              << queueFamilyIndex << "; skipping screenshot.";
    stagedFrame->IsReservedForDump.store(false);
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_StagedFrameFreeCV.notify_one();
    }
    return false;
  }
  VkCommandBuffer commandBuffer = commandBufferIt->second;

  stagedFrame->DumpName = dumpName;
  m_DispatchTable.vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkResult result = m_DispatchTable.vkBeginCommandBuffer(commandBuffer, &beginInfo);
  GITS_ASSERT(result == VK_SUCCESS);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchainImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  m_DispatchTable.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                       &barrier);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {m_Width, m_Height, 1};

  m_DispatchTable.vkCmdCopyImageToBuffer(commandBuffer, swapchainImage,
                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                         stagedFrame->StagingBuffer, 1, &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;

  m_DispatchTable.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                                       nullptr, 1, &barrier);

  result = m_DispatchTable.vkEndCommandBuffer(commandBuffer);
  GITS_ASSERT(result == VK_SUCCESS);

  m_DispatchTable.vkResetFences(m_Device, 1, &stagedFrame->Fence);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.waitSemaphoreCount = waitSemaphoreCount;
  submitInfo.pWaitSemaphores = pWaitSemaphores;
  std::vector<VkPipelineStageFlags> waitStages(waitSemaphoreCount, VK_PIPELINE_STAGE_TRANSFER_BIT);
  submitInfo.pWaitDstStageMask = waitStages.data();

  result = m_DispatchTable.vkQueueSubmit(queue, 1, &submitInfo, stagedFrame->Fence);
  GITS_ASSERT(result == VK_SUCCESS);

  // Only hand the frame to the worker pool once its copy has actually been
  // submitted, so a worker never waits on a stale (already signaled) fence.
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_SubmittedFrames.push(stagedFrame);
    m_FrameCopySubmittedCV.notify_one();
  }
  return true;
}

void SwapchainImagesDumper::DumpStagedFrame(StagedFrame& stagedFrame, const std::string& dumpName) {
  constexpr uint64_t oneSecondNanos = 1000000000ULL;
  VkResult result;
  do {
    result =
        m_DispatchTable.vkWaitForFences(m_Device, 1, &stagedFrame.Fence, VK_TRUE, oneSecondNanos);
  } while (result == VK_TIMEOUT);
  GITS_ASSERT(result == VK_SUCCESS);

  void* data = nullptr;
  result =
      m_DispatchTable.vkMapMemory(m_Device, stagedFrame.StagingMemory, 0, VK_WHOLE_SIZE, 0, &data);
  GITS_ASSERT(result == VK_SUCCESS);

  std::vector<uint8_t> localCopy;
  uint8_t* pixels = nullptr;

  if (m_StagingMemoryIsCached) {
    pixels = static_cast<uint8_t*>(data);
  } else {
    // Make a local copy of GPU data to improve performance
    const size_t imageSize = m_Width * m_Height * BYTES_PER_PIXEL;
    localCopy.resize(imageSize);
    std::memcpy(localCopy.data(), data, imageSize);
    pixels = localCopy.data();
  }

  bool isBGR = (m_Format == VK_FORMAT_B8G8R8A8_UNORM || m_Format == VK_FORMAT_B8G8R8A8_SRGB ||
                m_Format == VK_FORMAT_B8G8R8A8_SNORM);

  if (isBGR) {
    for (uint32_t i = 0; i < m_Width * m_Height; ++i) {
      std::swap(pixels[i * BYTES_PER_PIXEL + 0], pixels[i * BYTES_PER_PIXEL + 2]);
    }
  }

  // Force opaque alpha
  for (uint32_t i = 0; i < m_Width * m_Height; ++i) {
    pixels[i * BYTES_PER_PIXEL + 3] = 0xFF;
  }

  int saved = 0;
  std::string filename;
  if (Configurator::Get().common.shared.screenshots.format == ImageFormat::PNG) {
    filename = dumpName + ".png";
    // Note: stb image does not embed SRGB chunk, but it should work fine in most image viewers
    saved = stbi_write_png(filename.c_str(), static_cast<int>(m_Width), static_cast<int>(m_Height),
                           BYTES_PER_PIXEL, pixels, static_cast<int>(m_Width * BYTES_PER_PIXEL));
  } else {
    filename = dumpName + ".jpg";
    constexpr int jpegQuality = 95;
    saved = stbi_write_jpg(filename.c_str(), static_cast<int>(m_Width), static_cast<int>(m_Height),
                           BYTES_PER_PIXEL, pixels, jpegQuality);
  }

  if (!saved) {
    LOG_ERROR << "ScreenshotLayer: failed to write file: " << filename << " errno=" << errno << " ("
              << std::strerror(errno) << ")";
  }
  m_DispatchTable.vkUnmapMemory(m_Device, stagedFrame.StagingMemory);
}

void SwapchainImagesDumper::WorkerThread() {
  while (true) {
    StagedFrame* submittedFrame = nullptr;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_FrameCopySubmittedCV.wait(lock,
                                  [this] { return !m_SubmittedFrames.empty() || m_Shutdown; });

      // Drain any remaining submitted frames even during shutdown so the last
      // screenshots are flushed before the worker exits.
      if (m_SubmittedFrames.empty()) {
        return;
      }

      submittedFrame = m_SubmittedFrames.front();
      m_SubmittedFrames.pop();
    }
    if (m_Shutdown) {
      LOG_WARNING << "WorkerThread: shutdown is in progress, DumpStagedFrame is executing";
    }

    DumpStagedFrame(*submittedFrame, submittedFrame->DumpName);
    submittedFrame->IsReservedForDump.store(false);
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_StagedFrameFreeCV.notify_one();
    }
  }
}

} // namespace vulkan
} // namespace gits
