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

  void AllocateBuffers();
  void StartWorkerThread();

  void SubmitCommandBuffer(VkQueue queue,
                           VkImage swapchainImage,
                           const std::string& dumpedImageName);

private:
  struct StagedFrame {
    VkCommandBuffer CommandBuffer{};
    VkFence Fence{};
    VkBuffer StagingBuffer{};
    VkDeviceMemory StagingMemory{};
    std::string DumpName;
    std::atomic_bool IsReservedForDump{false};
  };

  StagedFrame* ReserveStagedFrame();
  StagedFrame* GetReservedStagedFrame();

  uint32_t GetMemoryTypeIndexWithFlags(uint32_t memoryTypeBits,
                                       VkMemoryPropertyFlags requiredFlags) const;
  void CreateStagingBuffer(StagedFrame& stagedFrame);
  void DumpStagedFrame(StagedFrame& stagedFrame, const std::string& dumpedImageName);
  void WorkerThread();

private:
  VkDeviceLevelDispatchTable& m_DispatchTable;
  VkDevice m_Device{};
  VkPhysicalDeviceMemoryProperties m_MemProperties{};
  VkCommandPool m_CommandPool{};
  VkFormat m_Format{};
  uint32_t m_Width{};
  uint32_t m_Height{};
  bool m_StagingMemoryIsCached{false};
  std::array<StagedFrame, MAX_STAGED_FRAMES> m_StagedFrames;

  std::thread m_Worker;
  std::mutex m_Mutex;
  std::condition_variable m_FrameCopySubmittedCV;
  std::condition_variable m_StagedFrameFreeCV;
  bool m_Shutdown{};
};

} // namespace vulkan
} // namespace gits
