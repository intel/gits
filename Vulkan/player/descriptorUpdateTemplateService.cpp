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

// Reads a GITSKey from the data buffer at byteOffset, resolves it to a
// player-side handle via TryGetHandle (returns VK_NULL_HANDLE if not mapped),
// and writes the handle back at the same offset.
// Reads a GITSKey from the data buffer at byteOffset, resolves it to a
// player-side handle via GetHandle, and writes the handle back at the same offset.
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
  auto rawHandle = HandleMapService::Get().GetHandle(key);
  auto playerHandle = reinterpret_cast<HandleT>(rawHandle);
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

  // Copy Data into PatchedData so we can substitute GITSKeys with player-side
  // handles without touching Data.  Data must remain intact (containing the
  // original GITSKeys) so that RecordingLayer can serialise it correctly into
  // the subcapture stream - if we patched Data in-place the live commands in
  // the subcapture stream would contain first-player handle values instead of
  // GITSKeys, causing the second player to misinterpret them.
  arg.PatchedData = arg.Data;

  for (const auto& entry : it->second.entries) {
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
          RemapHandle<VkSampler>(arg.PatchedData, samplerOffset);
        }
        if (entry.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
          RemapHandle<VkImageView>(arg.PatchedData, imageViewOffset);
        }
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
        size_t bufferOffset = base + offsetof(VkDescriptorBufferInfo, buffer);
        RemapHandle<VkBuffer>(arg.PatchedData, bufferOffset);
        break;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
        RemapHandle<VkBufferView>(arg.PatchedData, base);
        break;
      }
      default:
        break;
      }
    }
  }

  // Value points into PatchedData (player handles) for the Vulkan call.
  // Data is left intact (GITSKeys) for any subsequent serialization.
  arg.Value = arg.PatchedData.data();
}

} // namespace vulkan
} // namespace gits
