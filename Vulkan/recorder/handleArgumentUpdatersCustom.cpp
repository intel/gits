// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleArgumentUpdatersCustom.h"
#include "handleArgumentUpdatersAuto.h"
#include "vulkanHelpers.h"

namespace gits {
namespace vulkan {

/////////////////////////////////////////////////////////////////////////////////////////////////

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkWriteDescriptorSet& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.dstSet)));
  if (s.descriptorCount == 0) {
    return;
  }

  if (IsImageDescriptorType(s)) {
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
  if (IsBufferDescriptorType(s)) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      const auto& elem = s.pBufferInfo[elemIdx];
      keys.push_back(
          HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(elem.buffer)));
    }
  }
  if (IsTexelBufferDescriptorType(s)) {
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount; ++handleIdx) {
      keys.push_back(HandleMapService::Get().GetKeyLenient(
          reinterpret_cast<uint64_t>(s.pTexelBufferView[handleIdx])));
    }
  }
  if (IsAccelerationStructureDescriptorType(s)) {
    auto* pASWrite = (VkWriteDescriptorSetAccelerationStructureKHR*)getPNextStructure(
        s.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount; ++handleIdx) {
      keys.push_back(HandleMapService::Get().GetKeyLenient(
          reinterpret_cast<uint64_t>(pASWrite->pAccelerationStructures[handleIdx])));
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

/////////////////////////////////////////////////////////////////////////////////////////////////

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkPushDescriptorSetInfo& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.layout)));
  if (s.pDescriptorWrites && s.descriptorWriteCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorWriteCount; ++elemIdx) {
      CollectHandleKeys(keys, s.pDescriptorWrites[elemIdx]);
    }
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

/////////////////////////////////////////////////////////////////////////////////////////////////

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkImageCreateInfo& s) {}

void UpdateHandle(CaptureManager& manager, PointerArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
  CollectPNextHandleKeys(arg.HandleKeys, arg.Value->pNext);
}

void UpdateHandle(CaptureManager& manager, ArrayArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectHandleKeys(arg.HandleKeys, arg.Value[i]);
    CollectPNextHandleKeys(arg.HandleKeys, arg.Value[i].pNext);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkRayTracingPipelineCreateInfoKHR& s) {
  if (s.pStages && s.stageCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.stageCount; ++elemIdx) {
      const auto& elem = s.pStages[elemIdx];
      keys.push_back(
          HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(elem.module)));
    }
  }
  if (s.pLibraryInfo) {
    const auto& elem = *s.pLibraryInfo;
    if (elem.pLibraries && elem.libraryCount > 0) {
      for (uint32_t handleIdx = 0; handleIdx < elem.libraryCount; ++handleIdx) {
        keys.push_back(HandleMapService::Get().GetKeyLenient(
            reinterpret_cast<uint64_t>(elem.pLibraries[handleIdx])));
      }
    }
  }
  keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.layout)));
  keys.push_back(
      HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(s.basePipelineHandle)));
}

void UpdateHandle(CaptureManager& manager,
                  PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
  CollectPNextHandleKeys(arg.HandleKeys, arg.Value->pNext);
}

void UpdateHandle(CaptureManager& manager, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectHandleKeys(arg.HandleKeys, arg.Value[i]);
    CollectPNextHandleKeys(arg.HandleKeys, arg.Value[i].pNext);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

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
