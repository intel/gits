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
                       VkWriteDescriptorSet& s,
                       const HandleResolver& resolver) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.dstSet = key ? reinterpret_cast<VkDescriptorSet>(resolver.Resolve<VkDescriptorSet>(key))
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
          elem.sampler =
              key ? reinterpret_cast<VkSampler>(resolver.Resolve<VkSampler>(key)) : VK_NULL_HANDLE;
        }
      }
      if (s.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          elem.imageView = key ? reinterpret_cast<VkImageView>(resolver.Resolve<VkImageView>(key))
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
        elem.buffer =
            key ? reinterpret_cast<VkBuffer>(resolver.Resolve<VkBuffer>(key)) : VK_NULL_HANDLE;
      }
    }
  }
  if (IsTexelBufferDescriptorType(s)) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.descriptorCount);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = resolver.Resolve<VkBufferView>(key);
    }
    s.pTexelBufferView = reinterpret_cast<VkBufferView*>(&handleData[dataOffset]);
  }
  if (IsAccelerationStructureDescriptorType(s)) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.descriptorCount);
    for (uint32_t handleIdx = 0; handleIdx < s.descriptorCount && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = resolver.Resolve<VkAccelerationStructureKHR>(key);
    }
    ((VkWriteDescriptorSetAccelerationStructureKHR*)getPNextStructure(
         s.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR))
        ->pAccelerationStructures =
        reinterpret_cast<VkAccelerationStructureKHR*>(&handleData[dataOffset]);
  }
}

void UpdateHandle(PointerArgument<VkWriteDescriptorSet>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
}

void UpdateHandle(ArrayArgument<VkWriteDescriptorSet>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkPushDescriptorSetInfo& s,
                       const HandleResolver& resolver) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.layout = key ? reinterpret_cast<VkPipelineLayout>(resolver.Resolve<VkPipelineLayout>(key))
                   : VK_NULL_HANDLE;
  }
  if (s.pDescriptorWrites && s.descriptorWriteCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.descriptorWriteCount; ++elemIdx) {
      ResolveHandleKeys(keys, idx, handleData,
                        const_cast<VkWriteDescriptorSet&>(s.pDescriptorWrites[elemIdx]), resolver);
    }
  }
}

void UpdateHandle(PointerArgument<VkPushDescriptorSetInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
}

void UpdateHandle(ArrayArgument<VkPushDescriptorSetInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageCreateInfo& s,
                       const HandleResolver& resolver) {}

void UpdateHandle(PointerArgument<VkImageCreateInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkImageCreateInfo>& arg, const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkBufferCreateInfo& s,
                       const HandleResolver& resolver) {}

void UpdateHandle(PointerArgument<VkBufferCreateInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkBufferCreateInfo>& arg, const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageViewCreateInfo& s,
                       const HandleResolver& resolver) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.image = key ? reinterpret_cast<VkImage>(resolver.Resolve<VkImage>(key)) : VK_NULL_HANDLE;
  }
}

void UpdateHandle(PointerArgument<VkImageViewCreateInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkImageViewCreateInfo>& arg, const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkSamplerCreateInfo& s,
                       const HandleResolver& resolver) {}

void UpdateHandle(PointerArgument<VkSamplerCreateInfo>& arg, const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkSamplerCreateInfo>& arg, const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureCreateInfoKHR& s,
                       const HandleResolver& resolver) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.buffer = key ? reinterpret_cast<VkBuffer>(resolver.Resolve<VkBuffer>(key)) : VK_NULL_HANDLE;
  }
}

void UpdateHandle(PointerArgument<VkAccelerationStructureCreateInfoKHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkAccelerationStructureCreateInfoKHR>& arg,
                  const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureCreateInfo2KHR& s,
                       const HandleResolver& resolver) {}

void UpdateHandle(PointerArgument<VkAccelerationStructureCreateInfo2KHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkAccelerationStructureCreateInfo2KHR>& arg,
                  const HandleResolver& resolver) {
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
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkRayTracingPipelineCreateInfoKHR& s,
                       const HandleResolver& resolver) {

  if (s.pStages && s.stageCount > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.stageCount; ++elemIdx) {
      auto& elem = const_cast<VkPipelineShaderStageCreateInfo&>(s.pStages[elemIdx]);
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.module = key ? reinterpret_cast<VkShaderModule>(resolver.Resolve<VkShaderModule>(key))
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
        handleData[dataOffset + handleIdx] = resolver.Resolve<VkPipeline>(key);
      }
      elem.pLibraries = reinterpret_cast<VkPipeline*>(&handleData[dataOffset]);
    }
  }
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.layout = key ? reinterpret_cast<VkPipelineLayout>(resolver.Resolve<VkPipelineLayout>(key))
                   : VK_NULL_HANDLE;
  }
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.basePipelineHandle =
        key ? reinterpret_cast<VkPipeline>(resolver.Resolve<VkPipeline>(key)) : VK_NULL_HANDLE;
  }
}

void UpdateHandle(PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext, resolver);
}

void UpdateHandle(ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext, resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateOutputHandle(HandleArrayOutputArgument<VkPhysicalDevice>& arg) {
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

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

void ResolveAsBuildPairKeys(const std::vector<GITSKey>& keys,
                            uint32_t& idx,
                            VkAccelerationStructureBuildGeometryInfoKHR& s,
                            const HandleResolver& resolver) {
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.srcAccelerationStructure = key ? reinterpret_cast<VkAccelerationStructureKHR>(
                                           resolver.Resolve<VkAccelerationStructureKHR>(key))
                                     : VK_NULL_HANDLE;
  }
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.dstAccelerationStructure = key ? reinterpret_cast<VkAccelerationStructureKHR>(
                                           resolver.Resolve<VkAccelerationStructureKHR>(key))
                                     : VK_NULL_HANDLE;
  }
}

void ResolveAsBuildPNextKeys(const std::vector<GITSKey>& keys,
                             uint32_t& idx,
                             std::vector<uint64_t>& handleData,
                             VkAccelerationStructureBuildGeometryInfoKHR& s,
                             const HandleResolver& resolver) {
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
    ResolvePNextHandleKeys(keys, idx, handleData, geometry->geometry.triangles.pNext, resolver);
  }
  ResolvePNextHandleKeys(keys, idx, handleData, s.pNext, resolver);
}

} // namespace

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureBuildGeometryInfoKHR& s,
                       const HandleResolver& resolver) {
  ResolveAsBuildPairKeys(keys, idx, s, resolver);
  ResolveAsBuildPNextKeys(keys, idx, handleData, s, resolver);
}

void UpdateHandle(PointerArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value, resolver);
}

void UpdateHandle(ArrayArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg,
                  const HandleResolver& resolver) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
  arg.HandleData.reserve(arg.HandleKeys.size());
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveAsBuildPairKeys(arg.HandleKeys, idx, arg.Value[i], resolver);
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveAsBuildPNextKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i], resolver);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateOutputHandle(ArrayArgument<VkPhysicalDeviceGroupProperties>& arg) {
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
