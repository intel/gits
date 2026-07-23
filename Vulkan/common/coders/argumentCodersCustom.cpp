// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentCodersCustom.h"
#include "argumentCodersAuto.h"
#include "argumentCoders.h"
#include "handleMapService.h"

namespace gits {
namespace vulkan {

// Returns true if descriptorType uses pImageInfo.
static bool UsesImageInfo(VkDescriptorType type) {
  switch (type) {
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    return true;
  default:
    return false;
  }
}

// Returns true if descriptorType uses pBufferInfo.
static bool UsesBufferInfo(VkDescriptorType type) {
  switch (type) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    return true;
  default:
    return false;
  }
}

// Returns true if descriptorType uses pTexelBufferView.
static bool UsesTexelBufferView(VkDescriptorType type) {
  switch (type) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    return true;
  default:
    return false;
  }
}

uint32_t GetSize(const VkWriteDescriptorSet* src, uint32_t count) {
  if (!src) {
    return 0;
  }
  auto blobSize = static_cast<uint32_t>(sizeof(VkWriteDescriptorSet) * count);
  for (uint32_t i = 0; i < count; ++i) {
    const auto* desc = &src[i];
    blobSize += GetPNextChainSizeInput(desc->pNext);
    // dstSet key is in HandleKeys (collected by CollectHandleKeys) - no inline blob entry.
    if (desc->descriptorCount > 0) {
      if (UsesImageInfo(desc->descriptorType) && desc->pImageInfo) {
        blobSize += GetSize(desc->pImageInfo, desc->descriptorCount);
      } else if (UsesBufferInfo(desc->descriptorType) && desc->pBufferInfo) {
        blobSize += GetSize(desc->pBufferInfo, desc->descriptorCount);
      }
      // pTexelBufferView keys are in HandleKeys - no inline blob entries.
    }
  }
  return blobSize;
}

void Encode(const VkWriteDescriptorSet* src, uint32_t count, char* dst, uint32_t& offset) {
  if (!src || !dst) {
    return;
  }
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<VkWriteDescriptorSet*>(&dst[offset]);
  WriteData(reinterpret_cast<const char*>(src), sizeof(VkWriteDescriptorSet) * count, dst, offset);

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<VkWriteDescriptorSet*>(&srcDesc[i]);
    auto* currentDstDesc = &dstDesc[i];

    if (currentSrcDesc->pNext) {
      currentDstDesc->pNext =
          reinterpret_cast<decltype(currentDstDesc->pNext)>(static_cast<uintptr_t>(offset));
      EncodePNextChainInput(dst, offset, currentSrcDesc->pNext);
    } else {
      currentDstDesc->pNext = nullptr;
    }

    // dstSet key is collected into HandleKeys by CollectHandleKeys - no inline
    // blob entry here.  ResolveHandleKeys (player side) reads it from HandleKeys.

    if (currentSrcDesc->descriptorCount > 0) {
      if (UsesImageInfo(currentSrcDesc->descriptorType) && currentSrcDesc->pImageInfo) {
        currentDstDesc->pImageInfo =
            reinterpret_cast<VkDescriptorImageInfo*>(static_cast<uintptr_t>(offset));
        Encode(currentSrcDesc->pImageInfo, currentSrcDesc->descriptorCount, dst, offset);
      } else if (UsesBufferInfo(currentSrcDesc->descriptorType) && currentSrcDesc->pBufferInfo) {
        currentDstDesc->pBufferInfo =
            reinterpret_cast<VkDescriptorBufferInfo*>(static_cast<uintptr_t>(offset));
        Encode(currentSrcDesc->pBufferInfo, currentSrcDesc->descriptorCount, dst, offset);
      }
      // pTexelBufferView keys are collected into HandleKeys by CollectHandleKeys.
      // ResolveHandleKeys allocates handleData and redirects pTexelBufferView -
      // no inline blob entry needed here.
    }
  }
}

void Decode(const VkWriteDescriptorSet* dst, uint32_t count, char* src, uint32_t& offset) {
  offset += sizeof(VkWriteDescriptorSet) * count;

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<VkWriteDescriptorSet*>(&dst[i]);

    if (currentDstDesc->pNext) {
      DecodePNextChainInput(src, offset, const_cast<void**>(&currentDstDesc->pNext));
    }

    // dstSet is resolved from HandleKeys by ResolveHandleKeys - no inline blob key to read.

    if (currentDstDesc->descriptorCount > 0) {
      if (UsesImageInfo(currentDstDesc->descriptorType) && currentDstDesc->pImageInfo) {
        currentDstDesc->pImageInfo = AddPtrs(currentDstDesc->pImageInfo, src);
        Decode(currentDstDesc->pImageInfo, currentDstDesc->descriptorCount, src, offset);
      } else if (UsesBufferInfo(currentDstDesc->descriptorType) && currentDstDesc->pBufferInfo) {
        currentDstDesc->pBufferInfo = AddPtrs(currentDstDesc->pBufferInfo, src);
        Decode(currentDstDesc->pBufferInfo, currentDstDesc->descriptorCount, src, offset);
      }
      // pTexelBufferView is resolved from HandleKeys by ResolveHandleKeys -
      // no inline blob data to decode here.
    }
  }
}

// PointerArgument overloads for VkWriteDescriptorSet
uint32_t GetSize(const PointerArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + GetSize(arg.Value, 1) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst, uint32_t& offset, const PointerArgument<VkWriteDescriptorSet>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  Encode(arg.Value, 1, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, PointerArgument<VkWriteDescriptorSet>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  arg.Value = reinterpret_cast<VkWriteDescriptorSet*>(src + offset);
  Decode(arg.Value, 1, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

// ArrayArgument overloads for VkWriteDescriptorSet
uint32_t GetSize(const ArrayArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + GetSize(arg.Value, arg.Size) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkWriteDescriptorSet>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  std::memcpy(dst + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  Encode(arg.Value, arg.Size, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, ArrayArgument<VkWriteDescriptorSet>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = reinterpret_cast<VkWriteDescriptorSet*>(src + offset);
  Decode(arg.Value, arg.Size, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

// DescriptorTemplateDataArgument coders
// The Data vector is filled during capture (Pre layer) with the serialized descriptor entries.
// We encode its size followed by its raw bytes; on decode we restore the bytes and point Value
// into the buffer so the player call receives the correct data.
uint32_t GetSize(const DescriptorTemplateDataArgument& arg) {
  return sizeof(uint32_t) + static_cast<uint32_t>(arg.Data.size());
}

void Encode(char* dst, uint32_t& offset, const DescriptorTemplateDataArgument& arg) {
  uint32_t dataSize = static_cast<uint32_t>(arg.Data.size());
  std::memcpy(dst + offset, &dataSize, sizeof(dataSize));
  offset += sizeof(dataSize);
  if (dataSize > 0) {
    std::memcpy(dst + offset, arg.Data.data(), dataSize);
    offset += dataSize;
  }
}

void Decode(char* src, uint32_t& offset, DescriptorTemplateDataArgument& arg) {
  uint32_t dataSize = 0;
  std::memcpy(&dataSize, src + offset, sizeof(dataSize));
  offset += sizeof(dataSize);
  if (dataSize > 0) {
    arg.Data.resize(dataSize);
    std::memcpy(arg.Data.data(), src + offset, dataSize);
    offset += dataSize;
    arg.Value = arg.Data.data();
  } else {
    arg.Value = nullptr;
  }
}

static bool UsesConcurrentSharingMode(const VkImageCreateInfo* info) {
  return info && info->sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT &&
         info->pQueueFamilyIndices && info->queueFamilyIndexCount > 0;
}

uint32_t GetSize(const VkImageCreateInfo* src, uint32_t count) {
  if (!src) {
    return 0;
  }
  auto blobSize = static_cast<uint32_t>(sizeof(VkImageCreateInfo) * count);
  for (uint32_t i = 0; i < count; ++i) {
    const auto* imageInfo = &src[i];
    if (imageInfo->pNext) {
      blobSize += GetPNextChainSizeInput(imageInfo->pNext);
    }
    if (UsesConcurrentSharingMode(imageInfo)) {
      blobSize += sizeof(uint32_t) * imageInfo->queueFamilyIndexCount;
    }
  }
  return blobSize;
}

void Encode(const VkImageCreateInfo* src, uint32_t count, char* dst, uint32_t& offset) {
  if (!src || !dst) {
    return;
  }
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<VkImageCreateInfo*>(&dst[offset]);
  WriteData(reinterpret_cast<const char*>(src), sizeof(VkImageCreateInfo) * count, dst, offset);

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<VkImageCreateInfo*>(&srcDesc[i]);
    auto* currentDstDesc = &dstDesc[i];

    if (currentSrcDesc->pNext) {
      currentDstDesc->pNext =
          reinterpret_cast<decltype(currentDstDesc->pNext)>(static_cast<uintptr_t>(offset));
      EncodePNextChainInput(dst, offset, currentSrcDesc->pNext);
    } else {
      currentDstDesc->pNext = nullptr;
    }

    if (UsesConcurrentSharingMode(currentSrcDesc)) {
      currentDstDesc->pQueueFamilyIndices =
          reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(offset));
      std::memcpy(dst + offset, currentSrcDesc->pQueueFamilyIndices,
                  sizeof(uint32_t) * currentSrcDesc->queueFamilyIndexCount);
      offset += sizeof(uint32_t) * currentSrcDesc->queueFamilyIndexCount;
    } else {
      currentDstDesc->pQueueFamilyIndices = nullptr;
    }
  }
}

void Decode(const VkImageCreateInfo* dst, uint32_t count, char* src, uint32_t& offset) {
  offset += sizeof(VkImageCreateInfo) * count;

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<VkImageCreateInfo*>(&dst[i]);

    if (currentDstDesc->pNext) {
      DecodePNextChainInput(src, offset, const_cast<void**>(&currentDstDesc->pNext));
    }

    if (UsesConcurrentSharingMode(currentDstDesc)) {
      currentDstDesc->pQueueFamilyIndices = AddPtrs(currentDstDesc->pQueueFamilyIndices, src);
      offset += sizeof(uint32_t) * currentDstDesc->queueFamilyIndexCount;
    } else {
      currentDstDesc->pQueueFamilyIndices = nullptr;
    }
  }
}

// PointerArgument overloads for VkImageCreateInfo
uint32_t GetSize(const PointerArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + GetSize(arg.Value, 1) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst, uint32_t& offset, const PointerArgument<VkImageCreateInfo>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  Encode(arg.Value, 1, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, PointerArgument<VkImageCreateInfo>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  arg.Value = reinterpret_cast<VkImageCreateInfo*>(src + offset);
  Decode(arg.Value, 1, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

// ArrayArgument overloads for VkImageCreateInfo
uint32_t GetSize(const ArrayArgument<VkImageCreateInfo>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + GetSize(arg.Value, arg.Size) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkImageCreateInfo>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  std::memcpy(dst + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  Encode(arg.Value, arg.Size, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, ArrayArgument<VkImageCreateInfo>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = reinterpret_cast<VkImageCreateInfo*>(src + offset);
  Decode(arg.Value, arg.Size, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t GetSize(const VkRayTracingShaderGroupCreateInfoKHR* src, uint32_t count) {
  if (!src) {
    return 0;
  }
  auto blobSize = sizeof(VkRayTracingShaderGroupCreateInfoKHR) * count;
  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = &src[i];
    if (currentSrcDesc->pNext) {
      blobSize += GetPNextChainSizeInput(currentSrcDesc->pNext);
    }
    blobSize += sizeof(void*);
  }
  return blobSize;
}

void Encode(const VkRayTracingShaderGroupCreateInfoKHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset) {
  if (!src || !dst) {
    return;
  }
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<VkRayTracingShaderGroupCreateInfoKHR*>(&dst[offset]);
  WriteData(reinterpret_cast<const char*>(src),
            sizeof(VkRayTracingShaderGroupCreateInfoKHR) * count, dst, offset);

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<VkRayTracingShaderGroupCreateInfoKHR*>(&srcDesc[i]);
    auto* currentDstDesc = &dstDesc[i];
    if (currentSrcDesc->pNext) {
      currentDstDesc->pNext =
          reinterpret_cast<decltype(currentDstDesc->pNext)>(static_cast<uintptr_t>(offset));
      EncodePNextChainInput(dst, offset, currentSrcDesc->pNext);
    } else {
      currentDstDesc->pNext = nullptr;
    }
    {
      void* marker = const_cast<void*>(currentSrcDesc->pShaderGroupCaptureReplayHandle);
      std::memcpy(dst + offset, &marker, sizeof(void*));
      offset += sizeof(void*);
      currentDstDesc->pShaderGroupCaptureReplayHandle = nullptr;
    }
  }
}

void Decode(const VkRayTracingShaderGroupCreateInfoKHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset) {
  offset += sizeof(VkRayTracingShaderGroupCreateInfoKHR) * count;

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<VkRayTracingShaderGroupCreateInfoKHR*>(&dst[i]);
    if (currentDstDesc->pNext) {
      DecodePNextChainInput(src, offset, const_cast<void**>(&currentDstDesc->pNext));
    }
    {
      void* marker;
      std::memcpy(&marker, src + offset, sizeof(void*));
      offset += sizeof(void*);
      currentDstDesc->pShaderGroupCaptureReplayHandle = nullptr;
    }
  }
}

// PointerArgument overloads for VkRayTracingShaderGroupCreateInfoKHR
uint32_t GetSize(const PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + GetSize(arg.Value, 1) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  Encode(arg.Value, 1, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src,
            uint32_t& offset,
            PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  arg.Value = reinterpret_cast<VkRayTracingShaderGroupCreateInfoKHR*>(src + offset);
  Decode(arg.Value, 1, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

// ArrayArgument overloads for VkRayTracingShaderGroupCreateInfoKHR
uint32_t GetSize(const ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + GetSize(arg.Value, arg.Size) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  std::memcpy(dst + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  Encode(arg.Value, arg.Size, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = reinterpret_cast<VkRayTracingShaderGroupCreateInfoKHR*>(src + offset);
  Decode(arg.Value, arg.Size, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t GetSize(const VkRayTracingPipelineCreateInfoKHR* src, uint32_t count) {
  if (!src) {
    return 0;
  }
  auto blobSize = sizeof(VkRayTracingPipelineCreateInfoKHR) * count;
  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = &src[i];
    if (currentSrcDesc->pNext) {
      blobSize += GetPNextChainSizeInput(currentSrcDesc->pNext);
    }
    if (currentSrcDesc->pStages && currentSrcDesc->stageCount > 0) {
      blobSize += GetSize(currentSrcDesc->pStages, currentSrcDesc->stageCount);
    }
    if (currentSrcDesc->pGroups && currentSrcDesc->groupCount > 0) {
      blobSize += GetSize(currentSrcDesc->pGroups, currentSrcDesc->groupCount);
    }
    if (currentSrcDesc->pLibraryInfo) {
      blobSize += GetSize(currentSrcDesc->pLibraryInfo, 1);
    }
    if (currentSrcDesc->pLibraryInterface) {
      blobSize += GetSize(currentSrcDesc->pLibraryInterface, 1);
    }
    if (currentSrcDesc->pDynamicState) {
      blobSize += GetSize(currentSrcDesc->pDynamicState, 1);
    }
  }
  return blobSize;
}

void Encode(const VkRayTracingPipelineCreateInfoKHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset) {
  if (!src || !dst) {
    return;
  }
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>(&dst[offset]);
  WriteData(reinterpret_cast<const char*>(src), sizeof(VkRayTracingPipelineCreateInfoKHR) * count,
            dst, offset);

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<VkRayTracingPipelineCreateInfoKHR*>(&srcDesc[i]);
    auto* currentDstDesc = &dstDesc[i];
    if (currentSrcDesc->pNext) {
      currentDstDesc->pNext =
          reinterpret_cast<decltype(currentDstDesc->pNext)>(static_cast<uintptr_t>(offset));
      EncodePNextChainInput(dst, offset, currentSrcDesc->pNext);
    } else {
      currentDstDesc->pNext = nullptr;
    }
    if (currentSrcDesc->pStages && currentSrcDesc->stageCount > 0) {
      currentDstDesc->pStages =
          reinterpret_cast<VkPipelineShaderStageCreateInfo*>(static_cast<uintptr_t>(offset));
      Encode(currentSrcDesc->pStages, currentSrcDesc->stageCount, dst, offset);
    }
    if (currentSrcDesc->pGroups && currentSrcDesc->groupCount > 0) {
      currentDstDesc->pGroups =
          reinterpret_cast<VkRayTracingShaderGroupCreateInfoKHR*>(static_cast<uintptr_t>(offset));
      Encode(currentSrcDesc->pGroups, currentSrcDesc->groupCount, dst, offset);
    }
    if (currentSrcDesc->pLibraryInfo) {
      currentDstDesc->pLibraryInfo =
          reinterpret_cast<VkPipelineLibraryCreateInfoKHR*>(static_cast<uintptr_t>(offset));
      Encode(currentSrcDesc->pLibraryInfo, 1, dst, offset);
    }
    if (currentSrcDesc->pLibraryInterface) {
      currentDstDesc->pLibraryInterface =
          reinterpret_cast<VkRayTracingPipelineInterfaceCreateInfoKHR*>(
              static_cast<uintptr_t>(offset));
      Encode(currentSrcDesc->pLibraryInterface, 1, dst, offset);
    }
    if (currentSrcDesc->pDynamicState) {
      currentDstDesc->pDynamicState =
          reinterpret_cast<VkPipelineDynamicStateCreateInfo*>(static_cast<uintptr_t>(offset));
      Encode(currentSrcDesc->pDynamicState, 1, dst, offset);
    }
  }
}

void Decode(const VkRayTracingPipelineCreateInfoKHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset) {
  offset += sizeof(VkRayTracingPipelineCreateInfoKHR) * count;

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<VkRayTracingPipelineCreateInfoKHR*>(&dst[i]);
    if (currentDstDesc->pNext) {
      DecodePNextChainInput(src, offset, const_cast<void**>(&currentDstDesc->pNext));
    }
    if (currentDstDesc->pStages && currentDstDesc->stageCount > 0) {
      currentDstDesc->pStages = AddPtrs(currentDstDesc->pStages, src);
      Decode(currentDstDesc->pStages, currentDstDesc->stageCount, src, offset);
    }
    if (currentDstDesc->pGroups && currentDstDesc->groupCount > 0) {
      currentDstDesc->pGroups = AddPtrs(currentDstDesc->pGroups, src);
      Decode(currentDstDesc->pGroups, currentDstDesc->groupCount, src, offset);
    }
    if (currentDstDesc->pLibraryInfo) {
      currentDstDesc->pLibraryInfo = AddPtrs(currentDstDesc->pLibraryInfo, src);
      Decode(currentDstDesc->pLibraryInfo, 1, src, offset);
    }
    if (currentDstDesc->pLibraryInterface) {
      currentDstDesc->pLibraryInterface = AddPtrs(currentDstDesc->pLibraryInterface, src);
      Decode(currentDstDesc->pLibraryInterface, 1, src, offset);
    }
    if (currentDstDesc->pDynamicState) {
      currentDstDesc->pDynamicState = AddPtrs(currentDstDesc->pDynamicState, src);
      Decode(currentDstDesc->pDynamicState, 1, src, offset);
    }
  }
}

// PointerArgument overloads for VkRayTracingPipelineCreateInfoKHR
uint32_t GetSize(const PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + GetSize(arg.Value, 1) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey);
}

void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  Encode(arg.Value, 1, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

void Decode(char* src, uint32_t& offset, PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  arg.Value = reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>(src + offset);
  Decode(arg.Value, 1, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }
}

// ArrayArgument overloads for VkRayTracingPipelineCreateInfoKHR
uint32_t GetSize(const ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + GetSize(arg.Value, arg.Size) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.HandleKeys.size()) * sizeof(GITSKey) +
         sizeof(arg.CaptureReplayHandleSize) + sizeof(uint32_t) +
         static_cast<uint32_t>(arg.CaptureReplayHandlesData.size());
}

void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  std::memcpy(dst + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  Encode(arg.Value, arg.Size, dst, offset);
  uint32_t keyCount = static_cast<uint32_t>(arg.HandleKeys.size());
  std::memcpy(dst + offset, &keyCount, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    std::memcpy(dst + offset, arg.HandleKeys.data(), sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }

  uint32_t captureReplayHandleSize = static_cast<uint32_t>(arg.CaptureReplayHandleSize);
  std::memcpy(dst + offset, &captureReplayHandleSize, sizeof(captureReplayHandleSize));
  offset += sizeof(captureReplayHandleSize);

  uint32_t captureReplayHandlesDataSize =
      static_cast<uint32_t>(arg.CaptureReplayHandlesData.size());
  std::memcpy(dst + offset, &captureReplayHandlesDataSize, sizeof(captureReplayHandlesDataSize));
  offset += sizeof(captureReplayHandlesDataSize);

  if (captureReplayHandlesDataSize > 0) {
    std::memcpy(dst + offset, arg.CaptureReplayHandlesData.data(), captureReplayHandlesDataSize);
    offset += captureReplayHandlesDataSize;
  }
}

void Decode(char* src, uint32_t& offset, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = reinterpret_cast<VkRayTracingPipelineCreateInfoKHR*>(src + offset);
  Decode(arg.Value, arg.Size, src, offset);
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount);
  if (keyCount > 0) {
    arg.HandleKeys.resize(keyCount);
    std::memcpy(arg.HandleKeys.data(), src + offset, sizeof(GITSKey) * keyCount);
    offset += sizeof(GITSKey) * keyCount;
  }

  std::memcpy(&arg.CaptureReplayHandleSize, src + offset, sizeof(arg.CaptureReplayHandleSize));
  offset += sizeof(arg.CaptureReplayHandleSize);

  uint32_t captureReplayHandlesDataSize{};
  std::memcpy(&captureReplayHandlesDataSize, src + offset, sizeof(captureReplayHandlesDataSize));
  offset += sizeof(captureReplayHandlesDataSize);

  if (captureReplayHandlesDataSize > 0) {
    arg.CaptureReplayHandlesData.resize(captureReplayHandlesDataSize);
    std::memcpy(arg.CaptureReplayHandlesData.data(), src + offset, captureReplayHandlesDataSize);
    offset += captureReplayHandlesDataSize;
  }
}

} // namespace vulkan
} // namespace gits
