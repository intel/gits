// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "playerManager.h"
#include "handleMapService.h"

namespace gits {
namespace vulkan {

template <typename T>
void UpdateHandle(PlayerManager& manager, HandleArgument<T>& arg) {
  if (!arg.Key) {
    return;
  }
  arg.Value = reinterpret_cast<T>(HandleMapService::Get().GetHandle(arg.Key));
}

template <typename T>
void UpdateHandle(PlayerManager& manager, HandleArrayArgument<T>& arg) {
  for (uint32_t i = 0; i < arg.Keys.size(); ++i) {
    if (!arg.Keys[i]) {
      arg.Value[i] = VK_NULL_HANDLE;
    } else {
      arg.Value[i] = reinterpret_cast<T>(HandleMapService::Get().GetHandle(arg.Keys[i]));
    }
  }
}

template <typename T>
void UpdateOutputHandle(PlayerManager& manager, HandleOutputArgument<T>& arg) {
  if (!arg.Value || !*arg.Value) {
    return;
  }

  auto handle = reinterpret_cast<std::uint64_t>(*arg.Value);
  HandleMapService::Get().SetHandle(arg.Key, handle);
}

template <typename T>
void UpdateOutputHandle(PlayerManager& manager, HandleArrayOutputArgument<T>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    auto handle = reinterpret_cast<std::uint64_t>(arg.Value[i]);
    HandleMapService::Get().SetHandle(arg.Keys[i], handle);
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkCommandBufferAllocateInfo>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkDescriptorSetAllocateInfo>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkImageViewCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkBufferMemoryBarrier>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkPipelineLayoutCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkGraphicsPipelineCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkComputePipelineCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkComputePipelineCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkImageMemoryBarrier>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkCopyDescriptorSet>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSubmitInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSubmitInfo2>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkSwapchainCreateInfoKHR>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSwapchainCreateInfoKHR>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkFramebufferCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkRenderPassBeginInfo>& arg);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkPresentInfoKHR>& arg);

} // namespace vulkan
} // namespace gits
