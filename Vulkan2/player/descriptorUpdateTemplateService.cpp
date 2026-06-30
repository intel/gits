// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorUpdateTemplateService.h"
#include "handleMapService.h"

#include <cstring>

namespace gits {
namespace vulkan {

namespace {

// Reads a GITSKey from the data buffer at byteOffset and writes back the
// player-side handle obtained from HandleMapService.
template <typename HandleT>
void RemapHandle(std::vector<char>& data, size_t byteOffset) {
  GITS_ASSERT(sizeof(HandleT) == sizeof(GITSKey),
              "Handle size must match GITSKey size for in-place substitution");
  if (byteOffset + sizeof(GITSKey) > data.size()) {
    return;
  }
  GITSKey key{};
  std::memcpy(&key, data.data() + byteOffset, sizeof(GITSKey));
  if (!key) {
    return;
  }
  auto playerHandle = reinterpret_cast<HandleT>(HandleMapService::Get().GetHandle(key));
  std::memcpy(data.data() + byteOffset, &playerHandle, sizeof(HandleT));
}

} // namespace

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

void DescriptorUpdateTemplateService::RemapHandles(VkDescriptorUpdateTemplate tmpl,
                                                   DescriptorTemplateDataArgument& arg) {
  if (arg.Data.empty() || tmpl == VK_NULL_HANDLE) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Templates.find(tmpl);
  if (it == m_Templates.end()) {
    return;
  }

  for (const auto& entry : it->second.entries) {
    for (uint32_t i = 0; i < entry.descriptorCount; ++i) {
      size_t base = entry.offset + static_cast<size_t>(i) * entry.stride;
      switch (entry.descriptorType) {
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
        // VkDescriptorImageInfo: { VkSampler, VkImageView, VkImageLayout }
        size_t samplerOffset = base + offsetof(VkDescriptorImageInfo, sampler);
        size_t imageViewOffset = base + offsetof(VkDescriptorImageInfo, imageView);
        if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
            entry.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          RemapHandle<VkSampler>(arg.Data, samplerOffset);
        }
        if (entry.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
          RemapHandle<VkImageView>(arg.Data, imageViewOffset);
        }
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
        // VkDescriptorBufferInfo: { VkBuffer, VkDeviceSize offset, VkDeviceSize range }
        size_t bufferOffset = base + offsetof(VkDescriptorBufferInfo, buffer);
        RemapHandle<VkBuffer>(arg.Data, bufferOffset);
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
        // VkBufferView stored directly
        RemapHandle<VkBufferView>(arg.Data, base);
        break;
      }
      default:
        break;
      }
    }
  }

  // Value must point into the (now-patched) Data buffer
  arg.Value = arg.Data.data();
}

} // namespace vulkan
} // namespace gits
