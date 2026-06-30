// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "swapchainImagesDumper.h"
#include "dispatchTablesHolder.h"
#include "bit_range.h"

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

namespace gits {
namespace vulkan {

class ScreenshotsLayer : public Layer {
public:
  static constexpr const char* LAYER_NAME = "Screenshots";

  // Layers must use proper instace/device level dispatch tables.
  // Cannot use application dispatch table from vulkan-1.dll, because
  // GITS layer has its own dispatch table and wrapped handles.
  ScreenshotsLayer(DispatchTablesHolder& dispatchTablesHolder);

  void Post(vkCreateDeviceCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkCreateSwapchainKHRCommand& command) override;
  void Pre(vkDestroySwapchainKHRCommand& command) override;
  void Pre(vkQueuePresentKHRCommand& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;

private:
  struct SwapchainInfo {
    VkDevice Device{};
    std::vector<VkImage> Images;
    VkFormat Format{};
    VkExtent2D Extent{};
    std::unique_ptr<SwapchainImagesDumper> Dumper;
  };

  DispatchTablesHolder& m_DispatchTablesHolder;
  // Protect the shared maps
  std::mutex m_Mutex;
  std::unordered_map<VkDevice, VkPhysicalDevice> m_DeviceToPhysicalDevice;
  std::map<VkSwapchainKHR, SwapchainInfo> m_SwapchainInfos;
  std::unordered_map<VkQueue, uint32_t> m_QueueToFamily;
  BitRange m_ScreenshotRange;
  std::filesystem::path m_DumpPath;
  std::atomic<unsigned> m_CurrentFrame{};
};

} // namespace vulkan
} // namespace gits
