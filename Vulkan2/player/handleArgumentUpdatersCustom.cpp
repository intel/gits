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

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkWriteDescriptorSet& s) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.dstSet = key ? reinterpret_cast<VkDescriptorSet>(HandleMapService::Get().GetHandle(key))
                   : VK_NULL_HANDLE;
  }
  if (IsImageDescriptorType(s.descriptorType) && s.pImageInfo && s.descriptorCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      auto& elem = const_cast<VkDescriptorImageInfo&>(s.pImageInfo[elemIdx]);
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.sampler = key ? reinterpret_cast<VkSampler>(HandleMapService::Get().GetHandle(key))
                           : VK_NULL_HANDLE;
      }
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.imageView = key ? reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(key))
                             : VK_NULL_HANDLE;
      }
    }
  }
  if (IsBufferDescriptorType(s.descriptorType) && s.pBufferInfo && s.descriptorCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      auto& elem = const_cast<VkDescriptorBufferInfo&>(s.pBufferInfo[elemIdx]);
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.buffer = key ? reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(key))
                          : VK_NULL_HANDLE;
      }
    }
  }
  if (IsTexelBufferDescriptorType(s.descriptorType) && s.descriptorCount > 0) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.descriptorCount);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
    }
    s.pTexelBufferView = reinterpret_cast<VkBufferView*>(&handleData[dataOffset]);
  }
}

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkPushDescriptorSetInfo& s) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.layout = key ? reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(key))
                   : VK_NULL_HANDLE;
  }
  if (s.pDescriptorWrites && s.descriptorWriteCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorWriteCount; ++elemIdx) {
      ResolveHandleKeys(keys, idx, handleData,
                        const_cast<VkWriteDescriptorSet&>(s.pDescriptorWrites[elemIdx]));
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value);
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i]);
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkPushDescriptorSetInfo>& arg) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value);
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkPushDescriptorSetInfo>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i]);
  }
}

void UpdateOutputHandle(PlayerManager& manager,
                        ArrayArgument<VkPhysicalDeviceGroupProperties>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t keyIdx = 0;
  for (uint32_t i = 0; i < arg.Size; ++i) {
    const auto& group = arg.Value[i];
    for (uint32_t j = 0; j < group.physicalDeviceCount && keyIdx < arg.HandleKeys.size(); ++j) {
      if (group.physicalDevices[j] != VK_NULL_HANDLE) {
        auto handle = reinterpret_cast<uint64_t>(group.physicalDevices[j]);
        HandleMapService::Get().SetHandle(arg.HandleKeys[keyIdx], handle);
      }
      ++keyIdx;
    }
  }
}

} // namespace vulkan
} // namespace gits
