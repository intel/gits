// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleArgumentUpdatersCustom.h"

namespace gits {
namespace vulkan {

bool IsImageDescriptorType(VkDescriptorType type) {
  return type == VK_DESCRIPTOR_TYPE_SAMPLER || type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
         type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
         type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
}

bool IsBufferDescriptorType(VkDescriptorType type) {
  return type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
         type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
         type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

bool IsTexelBufferDescriptorType(VkDescriptorType type) {
  return type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
         type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
}

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkWriteDescriptorSet& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.dstSet)));
  if (IsImageDescriptorType(s.descriptorType) && s.pImageInfo && s.descriptorCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      const auto& elem = s.pImageInfo[elemIdx];
      if (s.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
          s.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
        keys.push_back(
            HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(elem.sampler)));
      }
      if (s.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
        keys.push_back(
            HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(elem.imageView)));
      }
    }
  }
  if (IsBufferDescriptorType(s.descriptorType) && s.pBufferInfo && s.descriptorCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      const auto& elem = s.pBufferInfo[elemIdx];
      keys.push_back(
          HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(elem.buffer)));
    }
  }
  if (IsTexelBufferDescriptorType(s.descriptorType) && s.pTexelBufferView &&
      s.descriptorCount > 0) {
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount; ++handleIdx) {
      keys.push_back(HandleMapService::Get().GetKeyLenient(
          reinterpret_cast<uint64_t>(s.pTexelBufferView[handleIdx])));
    }
  }
}

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkPushDescriptorSetInfo& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.layout)));
  if (s.pDescriptorWrites && s.descriptorWriteCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorWriteCount; ++elemIdx) {
      CollectHandleKeys(keys, s.pDescriptorWrites[elemIdx]);
    }
  }
}

void UpdateHandle(CaptureManager& manager, PointerArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
}

void UpdateHandle(CaptureManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectHandleKeys(arg.HandleKeys, arg.Value[i]);
  }
}

void UpdateHandle(CaptureManager& manager, PointerArgument<VkPushDescriptorSetInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
}

void UpdateHandle(CaptureManager& manager, ArrayArgument<VkPushDescriptorSetInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectHandleKeys(arg.HandleKeys, arg.Value[i]);
  }
}

void UpdateOutputHandle(CaptureManager& manager,
                        ArrayArgument<VkPhysicalDeviceGroupProperties>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    const auto& group = arg.Value[i];
    for (uint32_t j = 0; j < group.physicalDeviceCount; ++j) {
      VkPhysicalDevice device = group.physicalDevices[j];
      if (device == VK_NULL_HANDLE) {
        arg.HandleKeys.push_back(0);
        continue;
      }
      auto handle = reinterpret_cast<uint64_t>(device);
      if (!HandleMapService::Get().HasKey(handle)) {
        GITSKey key = manager.CreateHandleKey();
        HandleMapService::Get().SetKey(handle, key);
      }
      arg.HandleKeys.push_back(HandleMapService::Get().GetKey(handle));
    }
  }
}

} // namespace vulkan
} // namespace gits
