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

#include <filesystem>
#include <map>
#include <memory>
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
  void Post(vkCreateSwapchainKHRCommand& command) override;
  // Flush and release the swapchain's dumper before the swapchain (and shortly
  // after, the device) is destroyed, while both are still valid.  This both
  // captures the final present's screenshot for a normally-terminating stream
  // and guarantees the dumper never runs its Vulkan cleanup against a destroyed
  // device.
  void Pre(vkDestroySwapchainKHRCommand& command) override;
  void Pre(vkQueuePresentKHRCommand& command) override;

private:
  struct SwapchainInfo {
    VkDevice Device{};
    std::vector<VkImage> Images;
    VkFormat Format{};
    VkExtent2D Extent{};
    std::unique_ptr<SwapchainImagesDumper> Dumper;
  };

  DispatchTablesHolder& m_DispatchTablesHolder;
  std::unordered_map<VkDevice, VkPhysicalDevice> m_DeviceToPhysicalDevice;
  std::map<VkSwapchainKHR, SwapchainInfo> m_SwapchainInfos;
  BitRange m_ScreenshotRange;
  std::filesystem::path m_DumpPath;
  unsigned m_CurrentFrame{};
};

} // namespace vulkan
} // namespace gits
