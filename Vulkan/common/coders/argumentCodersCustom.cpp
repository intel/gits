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

} // namespace vulkan
} // namespace gits
