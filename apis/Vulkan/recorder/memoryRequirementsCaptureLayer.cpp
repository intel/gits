// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "memoryRequirementsCaptureLayer.h"
#include "handleMapService.h"
#include "log.h"
#include "vulkanHelpers.h"

#include <bit>
#include <limits>
#include <stdexcept>

namespace gits {
namespace vulkan {

namespace {

bool RequiresOrPrefersDedicatedAllocation(const VkMemoryRequirements2& requirements) {
  const auto* dedicated = static_cast<const VkMemoryDedicatedRequirements*>(
      getPNextStructure(requirements.pNext, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS));
  return dedicated &&
         (dedicated->requiresDedicatedAllocation || dedicated->prefersDedicatedAllocation);
}

VkDeviceSize CheckedAdd(VkDeviceSize base, VkDeviceSize increment) {
  if (increment > std::numeric_limits<VkDeviceSize>::max() - base) {
    throw std::overflow_error("Image memory requirement increase exceeds VkDeviceSize");
  }
  return base + increment;
}

} // namespace

bool MemoryRequirementsCaptureLayer::ProcessImageMemoryRequirements(
    VkMemoryRequirements& requirements) const {
  const VkDeviceSize originalSize = requirements.size;

  VkDeviceSize size = requirements.size;
  if ((m_Percent != 0) || (m_FixedAmount != 0)) {
    const VkDeviceSize whole = size / 100;
    if ((m_Percent != 0) && (whole > std::numeric_limits<VkDeviceSize>::max() / m_Percent)) {
      throw std::overflow_error("Image memory requirement increase exceeds VkDeviceSize");
    }
    const VkDeviceSize fraction = ((size % 100) * m_Percent + 99) / 100;
    size = CheckedAdd(size, CheckedAdd(whole * m_Percent, fraction));
    size = CheckedAdd(size, m_FixedAmount);
  }

  VkDeviceSize alignment = requirements.alignment;
  if (m_ImageAlignment != 0) {
    if (!std::has_single_bit(m_ImageAlignment) || (alignment == 0) ||
        (m_ImageAlignment < alignment) || (m_ImageAlignment % alignment != 0)) {
      LOG_WARNING << "Image memory offset alignment override " << m_ImageAlignment
                  << " must be a power of two and a multiple of the required alignment "
                  << alignment << "; override ignored.";
    } else {
      alignment = m_ImageAlignment;
    }
  }

  requirements.size = size;
  requirements.alignment = alignment;
  return size > originalSize;
}

void MemoryRequirementsCaptureLayer::ProcessImageMemoryRequirements(
    VkImage image, VkMemoryRequirements& requirements) {
  const bool sizeIncreased = ProcessImageMemoryRequirements(requirements);
  if (image && sizeIncreased) {
    const GITSKey imageKey = HandleMapService::Get().GetKeyLenient(image);
    if (imageKey) {
      std::lock_guard<std::mutex> lock(m_IncreasedImagesMutex);
      m_IncreasedImages.insert(imageKey);
    }
  }
}

void MemoryRequirementsCaptureLayer::WarnIfDedicatedAllocationWasIncreased(VkImage image) {
  if (!image) {
    return;
  }
  const GITSKey imageKey = HandleMapService::Get().GetKeyLenient(image);
  if (!imageKey) {
    return;
  }

  bool wasIncreased = false;
  {
    std::lock_guard<std::mutex> lock(m_IncreasedImagesMutex);
    wasIncreased = m_IncreasedImages.erase(imageKey) != 0;
  }
  if (wasIncreased) {
    LOG_WARNING << "Image (key " << imageKey
                << ") was reported with an increased memory requirement, but the application "
                   "later requested a dedicated allocation. The earlier requirement increase "
                   "could not be undone.";
  }
}

void MemoryRequirementsCaptureLayer::Post(vkGetImageMemoryRequirementsCommand& command) {
  if (command.m_Skip || !command.m_pMemoryRequirements.Value) {
    return;
  }

  ProcessImageMemoryRequirements(command.m_image.Value, *command.m_pMemoryRequirements.Value);
}

void MemoryRequirementsCaptureLayer::Post(vkGetImageMemoryRequirements2Command& command) {
  if (command.m_Skip || !command.m_pMemoryRequirements.Value ||
      RequiresOrPrefersDedicatedAllocation(*command.m_pMemoryRequirements.Value)) {
    return;
  }

  ProcessImageMemoryRequirements(command.m_pInfo.Value ? command.m_pInfo.Value->image
                                                       : VK_NULL_HANDLE,
                                 command.m_pMemoryRequirements.Value->memoryRequirements);
}

void MemoryRequirementsCaptureLayer::Post(vkGetImageMemoryRequirements2KHRCommand& command) {
  if (command.m_Skip || !command.m_pMemoryRequirements.Value ||
      RequiresOrPrefersDedicatedAllocation(*command.m_pMemoryRequirements.Value)) {
    return;
  }

  ProcessImageMemoryRequirements(command.m_pInfo.Value ? command.m_pInfo.Value->image
                                                       : VK_NULL_HANDLE,
                                 command.m_pMemoryRequirements.Value->memoryRequirements);
}

void MemoryRequirementsCaptureLayer::Post(vkGetDeviceImageMemoryRequirementsCommand& command) {
  if (command.m_Skip || !command.m_pMemoryRequirements.Value ||
      RequiresOrPrefersDedicatedAllocation(*command.m_pMemoryRequirements.Value)) {
    return;
  }

  ProcessImageMemoryRequirements(command.m_pMemoryRequirements.Value->memoryRequirements);
}

void MemoryRequirementsCaptureLayer::Post(vkGetDeviceImageMemoryRequirementsKHRCommand& command) {
  if (command.m_Skip || !command.m_pMemoryRequirements.Value ||
      RequiresOrPrefersDedicatedAllocation(*command.m_pMemoryRequirements.Value)) {
    return;
  }

  ProcessImageMemoryRequirements(command.m_pMemoryRequirements.Value->memoryRequirements);
}

void MemoryRequirementsCaptureLayer::Pre(vkAllocateMemoryCommand& command) {
  if (command.m_Skip || !command.m_pAllocateInfo.Value) {
    return;
  }

  const auto* dedicated = static_cast<const VkMemoryDedicatedAllocateInfo*>(getPNextStructure(
      command.m_pAllocateInfo.Value->pNext, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO));
  if (dedicated) {
    WarnIfDedicatedAllocationWasIncreased(dedicated->image);
  }
}

void MemoryRequirementsCaptureLayer::Post(vkDestroyImageCommand& command) {
  if (!command.m_Skip && command.m_image.Key) {
    std::lock_guard<std::mutex> lock(m_IncreasedImagesMutex);
    m_IncreasedImages.erase(command.m_image.Key);
  }
}

} // namespace vulkan
} // namespace gits
