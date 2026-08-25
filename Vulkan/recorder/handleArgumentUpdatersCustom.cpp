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

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkWriteDescriptorSet& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(s.dstSet));
  if (s.descriptorCount == 0) {
    return;
  }

  if (IsImageDescriptorType(s)) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      const auto& elem = s.pImageInfo[elemIdx];
      if (s.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
          s.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
        keys.push_back(HandleMapService::Get().GetKeyLenient(elem.sampler));
      }
      if (s.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
        keys.push_back(HandleMapService::Get().GetKeyLenient(elem.imageView));
      }
    }
  }
  if (IsBufferDescriptorType(s)) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorCount; ++elemIdx) {
      const auto& elem = s.pBufferInfo[elemIdx];
      keys.push_back(HandleMapService::Get().GetKeyLenient(elem.buffer));
    }
  }
  if (IsTexelBufferDescriptorType(s)) {
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount; ++handleIdx) {
      keys.push_back(HandleMapService::Get().GetKeyLenient(s.pTexelBufferView[handleIdx]));
    }
  }
  if (IsAccelerationStructureDescriptorType(s)) {
    auto* pASWrite = (VkWriteDescriptorSetAccelerationStructureKHR*)getPNextStructure(
        s.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount; ++handleIdx) {
      keys.push_back(
          HandleMapService::Get().GetKeyLenient(pASWrite->pAccelerationStructures[handleIdx]));
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
  keys.push_back(HandleMapService::Get().GetKeyLenient(s.layout));
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
      keys.push_back(HandleMapService::Get().GetKeyLenient(elem.module));
    }
  }
  if (s.pLibraryInfo) {
    const auto& elem = *s.pLibraryInfo;
    if (elem.pLibraries && elem.libraryCount > 0) {
      for (uint32_t handleIdx = 0; handleIdx < elem.libraryCount; ++handleIdx) {
        keys.push_back(HandleMapService::Get().GetKeyLenient(elem.pLibraries[handleIdx]));
      }
    }
  }
  keys.push_back(HandleMapService::Get().GetKeyLenient(s.layout));
  keys.push_back(HandleMapService::Get().GetKeyLenient(s.basePipelineHandle));
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

namespace {

void CollectAsBuildPairKeys(std::vector<GITSKey>& keys,
                            const VkAccelerationStructureBuildGeometryInfoKHR& s) {
  keys.push_back(HandleMapService::Get().GetKeyLenient(
      reinterpret_cast<uint64_t>(s.srcAccelerationStructure)));
  keys.push_back(HandleMapService::Get().GetKeyLenient(
      reinterpret_cast<uint64_t>(s.dstAccelerationStructure)));
}

// The variable part - a micromap handle chained onto a triangle geometry, plus the info's own chain
void CollectAsBuildPNextKeys(std::vector<GITSKey>& keys,
                             const VkAccelerationStructureBuildGeometryInfoKHR& s) {
  for (uint32_t i = 0; i < s.geometryCount; ++i) {
    const VkAccelerationStructureGeometryKHR* geometry = nullptr;
    if (s.pGeometries) {
      geometry = &s.pGeometries[i];
    } else if (s.ppGeometries) {
      geometry = s.ppGeometries[i];
    }
    if (!geometry || geometry->geometryType != VK_GEOMETRY_TYPE_TRIANGLES_KHR) {
      continue;
    }
    CollectPNextHandleKeys(keys, geometry->geometry.triangles.pNext);
  }
  CollectPNextHandleKeys(keys, s.pNext);
}

} // namespace

void CollectHandleKeys(std::vector<GITSKey>& keys,
                       const VkAccelerationStructureBuildGeometryInfoKHR& s) {
  CollectAsBuildPairKeys(keys, s);
  CollectAsBuildPNextKeys(keys, s);
}

void UpdateHandle(CaptureManager& manager,
                  PointerArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
}

void UpdateHandle(CaptureManager& manager,
                  ArrayArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  // Pairs first, so the subcapture can reach element i's source and destination by index without
  // walking the payload the elements before it collected. ResolveHandleKeys mirrors the split.
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectAsBuildPairKeys(arg.HandleKeys, arg.Value[i]);
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectAsBuildPNextKeys(arg.HandleKeys, arg.Value[i]);
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
      if (!HandleMapService::Get().HasKey(device)) {
        GITSKey key = manager.CreateHandleKey();
        HandleMapService::Get().SetKey(device, key);
      }
      arg.HandleKeys.push_back(HandleMapService::Get().GetKey(device));
    }
  }
}

} // namespace vulkan
} // namespace gits
