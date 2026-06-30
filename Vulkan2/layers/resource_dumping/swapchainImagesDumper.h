// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

struct VkDeviceLevelDispatchTable;

// Dumps images to disk for specified swapchain format
class SwapchainImagesDumper {
public:
  static constexpr uint32_t MAX_STAGED_FRAMES = 10;
  static constexpr uint32_t BYTES_PER_PIXEL = 4;

  SwapchainImagesDumper(VkDevice device,
                        const VkSwapchainCreateInfoKHR& swapchainCreateInfo,
                        const VkPhysicalDeviceMemoryProperties& memProperties,
                        VkDeviceLevelDispatchTable& dispatchTable);
  ~SwapchainImagesDumper();
  SwapchainImagesDumper(const SwapchainImagesDumper&) = delete;
  SwapchainImagesDumper& operator=(const SwapchainImagesDumper&) = delete;

  // Pre-creates a command pool and per-staged-frame command buffers for every
  // queue family the swapchain may be presented from.
  void AllocateBuffers(const std::vector<uint32_t>& queueFamilyIndices);
  void StartWorkerThread();

  // Records and submits the copy on the present queue, waiting on the given
  // semaphores (typically the present's original wait semaphores) before the
  // transfer read. Returns true if the copy was actually submitted - i.e. the
  // wait semaphores were consumed - and false if the dump was skipped.
  bool SubmitCommandBuffer(VkQueue queue,
                           uint32_t queueFamilyIndex,
                           VkImage swapchainImage,
                           const std::string& dumpedImageName,
                           uint32_t waitSemaphoreCount,
                           const VkSemaphore* pWaitSemaphores);

private:
  struct StagedFrame {
    // One command buffer per queue family
    std::unordered_map<uint32_t, VkCommandBuffer> CommandBuffers;
    VkFence Fence{};
    VkBuffer StagingBuffer{};
    VkDeviceMemory StagingMemory{};
    std::string DumpName;
    std::atomic_bool IsReservedForDump{false};
  };

  StagedFrame* ReserveStagedFrame();

  uint32_t GetMemoryTypeIndexWithFlags(uint32_t memoryTypeBits,
                                       VkMemoryPropertyFlags requiredFlags) const;
  void CreateStagingBuffer(StagedFrame& stagedFrame);
  void DumpStagedFrame(StagedFrame& stagedFrame, const std::string& dumpedImageName);
  void WorkerThread();

private:
  VkDeviceLevelDispatchTable& m_DispatchTable;
  VkDevice m_Device{};
  VkPhysicalDeviceMemoryProperties m_MemProperties{};
  // One command pool per queue family the swapchain is presented from.
  std::unordered_map<uint32_t, VkCommandPool> m_CommandPools;
  VkFormat m_Format{};
  uint32_t m_Width{};
  uint32_t m_Height{};
  bool m_StagingMemoryIsCached{false};
  std::array<StagedFrame, MAX_STAGED_FRAMES> m_StagedFrames;

  std::vector<std::thread> m_Workers;
  // Frames whose copy has been submitted and are awaiting dump to disk
  std::queue<StagedFrame*> m_SubmittedFrames;
  std::mutex m_Mutex;
  std::condition_variable m_FrameCopySubmittedCV;
  std::condition_variable m_StagedFrameFreeCV;
  bool m_Shutdown{};
};

} // namespace vulkan
} // namespace gits
