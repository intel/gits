// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorUpdateTemplateService.h"
#include "handleMapService.h"

#include <algorithm>
#include <cstring>

namespace gits {
namespace vulkan {

void DescriptorUpdateTemplateService::StoreTemplate(
    VkDescriptorUpdateTemplate tmpl, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo) {
  if (!pCreateInfo || tmpl == VK_NULL_HANDLE) {
    return;
  }
  TemplateInfo info;
  info.entries.assign(pCreateInfo->pDescriptorUpdateEntries,
                      pCreateInfo->pDescriptorUpdateEntries +
                          pCreateInfo->descriptorUpdateEntryCount);
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Templates[tmpl] = std::move(info);
}

void DescriptorUpdateTemplateService::RemoveTemplate(VkDescriptorUpdateTemplate tmpl) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Templates.erase(tmpl);
}

namespace {

template <typename HandleT>
void StoreKey(std::vector<char>& data, size_t byteOffset) {
  GITS_ASSERT(sizeof(HandleT) == sizeof(GITSKey),
              "Handle size must match GITSKey size for in-place substitution");
  if (byteOffset + sizeof(GITSKey) > data.size()) {
    return;
  }
  HandleT handle{};
  std::memcpy(&handle, data.data() + byteOffset, sizeof(HandleT));
  if (!handle) {
    return;
  }
  GITSKey key = HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(handle));
  std::memcpy(data.data() + byteOffset, &key, sizeof(GITSKey));
}

} // namespace

void DescriptorUpdateTemplateService::SerializeData(VkDescriptorUpdateTemplate tmpl,
                                                    const void* pData,
                                                    DescriptorTemplateDataArgument& arg) {
  if (!pData || tmpl == VK_NULL_HANDLE) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Templates.find(tmpl);
  if (it == m_Templates.end()) {
    return;
  }

  const auto& entries = it->second.entries;
  const char* src = static_cast<const char*>(pData);

  size_t bufferSize = 0;
  for (const auto& entry : entries) {
    if (entry.descriptorCount == 0) {
      continue;
    }
    size_t entryEnd = entry.offset + entry.stride * (entry.descriptorCount - 1);
    switch (entry.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      entryEnd += sizeof(VkDescriptorBufferInfo);
      break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      entryEnd += sizeof(VkBufferView);
      break;
    default:
      entryEnd += sizeof(VkDescriptorImageInfo);
      break;
    }
    bufferSize = std::max(bufferSize, entryEnd);
  }

  if (bufferSize == 0) {
    return;
  }

  // arg.Value keeps pointing at the original app memory so the driver
  // receives the real handles. arg.Data is an independent serialization
  // copy where handles are replaced with GITSKeys for the stream.
  arg.Value = const_cast<void*>(pData);
  arg.Data.assign(src, src + bufferSize);

  for (const auto& entry : entries) {
    for (uint32_t i = 0; i < entry.descriptorCount; ++i) {
      size_t base = entry.offset + static_cast<size_t>(i) * entry.stride;
      switch (entry.descriptorType) {
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
        size_t samplerOffset = base + offsetof(VkDescriptorImageInfo, sampler);
        size_t imageViewOffset = base + offsetof(VkDescriptorImageInfo, imageView);
        if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
            entry.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          StoreKey<VkSampler>(arg.Data, samplerOffset);
        }
        if (entry.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
          StoreKey<VkImageView>(arg.Data, imageViewOffset);
        }
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
        size_t bufferOffset = base + offsetof(VkDescriptorBufferInfo, buffer);
        StoreKey<VkBuffer>(arg.Data, bufferOffset);
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
        StoreKey<VkBufferView>(arg.Data, base);
        break;
      }
      default:
        break;
      }
    }
  }
}

} // namespace vulkan
} // namespace gits
