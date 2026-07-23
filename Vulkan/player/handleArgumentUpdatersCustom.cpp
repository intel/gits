// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleArgumentUpdatersCustom.h"
#include "handleArgumentUpdatersPlayerAuto.h"
#include "vulkanHelpers.h"

#include "log.h"

namespace gits {
namespace vulkan {

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkWriteDescriptorSet& s) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.dstSet = key ? reinterpret_cast<VkDescriptorSet>(HandleMapService::Get().GetHandle(key))
                   : VK_NULL_HANDLE;
  }
  if (s.descriptorCount == 0) {
    return;
  }
  if (IsImageDescriptorType(s)) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      auto& elem = const_cast<VkDescriptorImageInfo&>(s.pImageInfo[elemIdx]);
      if (s.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
          s.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          elem.sampler = key ? reinterpret_cast<VkSampler>(HandleMapService::Get().GetHandle(key))
                             : VK_NULL_HANDLE;
        }
      }
      if (s.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          elem.imageView =
              key ? reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(key))
                  : VK_NULL_HANDLE;
        }
      }
    }
  }
  if (IsBufferDescriptorType(s)) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      auto& elem = const_cast<VkDescriptorBufferInfo&>(s.pBufferInfo[elemIdx]);
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.buffer = key ? reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(key))
                          : VK_NULL_HANDLE;
      }
    }
  }
  if (IsTexelBufferDescriptorType(s)) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.descriptorCount);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
    }
    s.pTexelBufferView = reinterpret_cast<VkBufferView*>(&handleData[dataOffset]);
  }
  if (IsAccelerationStructureDescriptorType(s)) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.descriptorCount);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
    }
    ((VkWriteDescriptorSetAccelerationStructureKHR*)getPNextStructure(
         s.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR))
        ->pAccelerationStructures =
        reinterpret_cast<VkAccelerationStructureKHR*>(&handleData[dataOffset]);
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

///////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageCreateInfo& s) {}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext);
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  for (uint32_t i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].pNext) {
      arg.HandleData.reserve(arg.HandleKeys.size());
      break;
    }
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i]);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkRayTracingPipelineCreateInfoKHR& s) {

  if (s.pStages && s.stageCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.stageCount; ++elemIdx) {
      auto& elem = const_cast<VkPipelineShaderStageCreateInfo&>(s.pStages[elemIdx]);
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.module = key ? reinterpret_cast<VkShaderModule>(HandleMapService::Get().GetHandle(key))
                          : VK_NULL_HANDLE;
      }
    }
  }

  if (s.pLibraryInfo) {
    auto& elem = const_cast<VkPipelineLibraryCreateInfoKHR&>(*s.pLibraryInfo);
    if (elem.pLibraries && elem.libraryCount > 0) {
      size_t dataOffset = handleData.size();
      handleData.resize(handleData.size() + elem.libraryCount);
      for (uint32_t handleIdx = 0; handleIdx < elem.libraryCount && idx < keys.size();
           ++handleIdx) {
        GITSKey key = keys[idx++];
        handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
      }
      elem.pLibraries = reinterpret_cast<VkPipeline*>(&handleData[dataOffset]);
    }
  }
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.layout = key ? reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(key))
                   : VK_NULL_HANDLE;
  }
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.basePipelineHandle =
        key ? reinterpret_cast<VkPipeline>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext);
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i]);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateOutputHandle(PlayerManager& manager, HandleArrayOutputArgument<VkPhysicalDevice>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  // Clamp any surplus captured keys onto a valid physical device so replay
  // proceeds on the available GPU instead of crashing.
  uint64_t fallback = 0;
  for (uint32_t i = 0; i < arg.Size; ++i) {
    if (arg.Value[i]) {
      fallback = reinterpret_cast<uint64_t>(arg.Value[i]);
      break;
    }
  }
  if (!fallback) {
    LOG_ERROR << "vkEnumeratePhysicalDevices returned no physical devices on replay; "
                 "cannot map captured physical-device handles.";
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    const bool filled = arg.Value[i] != VK_NULL_HANDLE;
    if (!filled) {
      LOG_WARNING << "Replay enumerated fewer physical devices than capture; mapping "
                     "captured physical device #"
                  << i << " onto device #0.";
    }
    HandleMapService::Get().SetHandle(arg.Keys[i],
                                      filled ? reinterpret_cast<uint64_t>(arg.Value[i]) : fallback);
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
