// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <mutex>
#include <unordered_set>

namespace gits {
namespace vulkan {

class MemoryRequirementsCaptureLayer : public Layer {
public:
  MemoryRequirementsCaptureLayer(uint32_t fixedAmount,
                                 uint32_t percent,
                                 VkDeviceSize imageAlignment)
      : Layer("MemoryRequirementsCapture"),
        m_FixedAmount(fixedAmount),
        m_Percent(percent),
        m_ImageAlignment(imageAlignment) {}

  void Post(vkGetImageMemoryRequirementsCommand& command) override;
  void Post(vkGetImageMemoryRequirements2Command& command) override;
  void Post(vkGetImageMemoryRequirements2KHRCommand& command) override;
  void Post(vkGetDeviceImageMemoryRequirementsCommand& command) override;
  void Post(vkGetDeviceImageMemoryRequirementsKHRCommand& command) override;
  void Pre(vkAllocateMemoryCommand& command) override;
  void Post(vkDestroyImageCommand& command) override;

private:
  bool ProcessImageMemoryRequirements(VkMemoryRequirements& requirements) const;
  void ProcessImageMemoryRequirements(VkImage image, VkMemoryRequirements& requirements);
  void WarnIfDedicatedAllocationWasIncreased(VkImage image);

  uint32_t m_FixedAmount;
  uint32_t m_Percent;
  VkDeviceSize m_ImageAlignment;
  std::mutex m_IncreasedImagesMutex;
  std::unordered_set<GITSKey> m_IncreasedImages;
};

} // namespace vulkan
} // namespace gits
