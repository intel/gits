// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "commandIdsAuto.h"
#include "subcaptureRecorder.h"

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Tracks the last-written data for a single descriptor array element.
// All Vulkan handle references are stored as recorder-side keys so that
// validity can be checked against the live StateTrackingService at restore
// time (analogous to the old Vulkan RestoreDescriptorSetsUpdates helpers).
struct DescriptorElementData {
  VkDescriptorType DescriptorType{VK_DESCRIPTOR_TYPE_MAX_ENUM};

  // Image descriptors (SAMPLER, COMBINED_IMAGE_SAMPLER, SAMPLED_IMAGE,
  //                    STORAGE_IMAGE, INPUT_ATTACHMENT)
  uint64_t SamplerKey{};   // non-zero for SAMPLER / COMBINED_IMAGE_SAMPLER
  uint64_t ImageViewKey{}; // non-zero for all image types except SAMPLER
  VkImageLayout ImageLayout{VK_IMAGE_LAYOUT_UNDEFINED};

  // Buffer descriptors (UNIFORM_BUFFER, STORAGE_BUFFER, and dynamic variants)
  uint64_t BufferKey{};
  VkDeviceSize BufferOffset{};
  VkDeviceSize BufferRange{VK_WHOLE_SIZE};

  // Texel-buffer descriptors (UNIFORM_TEXEL_BUFFER, STORAGE_TEXEL_BUFFER)
  uint64_t BufferViewKey{};

  // Acceleration-structure descriptors (VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
  uint64_t AccelerationStructureKey{};
};

// Tracks all written array elements for one VkDescriptorSetLayoutBinding slot.
struct DescriptorBindingData {
  VkDescriptorType Type{VK_DESCRIPTOR_TYPE_MAX_ENUM};

  // For VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: raw byte storage indexed by
  // byte offset from the beginning of the block.
  std::vector<uint8_t> InlineUniformData;

  // For all other descriptor types: one entry per array element, indexed by
  // array element index.  Entries with DescriptorType == MAX_ENUM were never
  // explicitly written and are skipped during restore.
  std::vector<DescriptorElementData> Elements;
};

// Tracks descriptor-set write and copy state accumulated between creation and
// the subcapture boundary so that vkUpdateDescriptorSets calls can be
// re-emitted during state restore.
//
// Design mirrors the DX12 DescriptorService and the old Vulkan
// RestoreDescriptorSetsUpdates flow:
//   - TrackUpdate() is called from SubcaptureLayer::Post(vkUpdateDescriptorSetsCommand)
//   - TrackCopy() handles VkCopyDescriptorSet operations
//   - TrackTemplateUpdate() stores encoded template-update commands verbatim
//   - RemoveDescriptorSet() is called when a set is freed or its pool is reset
//   - RestoreUpdates() is called from StateTrackingService::RestoreDescriptorSets
//     to emit the accumulated writes into the subcapture stream
class DescriptorSetUpdateService {
public:
  // Process all writes and copies from a single vkUpdateDescriptorSets call.
  // writeHandleKeys / copyHandleKeys must be the HandleKeys arrays decoded from
  // the command (recorder-side keys), and must match the layout produced by
  // CollectHandleKeys(VkWriteDescriptorSet) / CollectHandleKeys(VkCopyDescriptorSet).
  void TrackUpdate(uint32_t descriptorWriteCount,
                   const VkWriteDescriptorSet* pDescriptorWrites,
                   const std::vector<uint64_t>& writeHandleKeys,
                   uint32_t descriptorCopyCount,
                   const VkCopyDescriptorSet* pDescriptorCopies,
                   const std::vector<uint64_t>& copyHandleKeys);

  // Store the descriptor-update-template entries so that template updates can
  // be expanded element-by-element into m_Updates (same as regular writes).
  void StoreTemplateEntries(uint64_t templateKey,
                            const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo);
  void RemoveTemplateEntries(uint64_t templateKey);

  // Expand a template update into m_Updates element-by-element.
  // pDataBytes is the serialised buffer (from DescriptorTemplateDataArgument::Data)
  // where handle-sized fields contain GITSKeys as written by the recorder's
  // DescriptorUpdateTemplateService::SerializeData.
  void TrackTemplateUpdate(uint64_t setKey,
                           uint64_t templateKey,
                           const std::vector<char>& pDataBytes);

  // Remove all tracked state for a descriptor set (freed / pool reset).
  void RemoveDescriptorSet(uint64_t setKey);

  // Emit vkUpdateDescriptorSets (and any stored template-update commands) into
  // the subcapture recorder for the given descriptor set.  Referenced objects
  // that no longer appear in the live StateTrackingService are silently skipped,
  // matching the old Vulkan behaviour.
  void RestoreUpdates(uint64_t setKey, SubcaptureRecorder& recorder, StateTrackingService& sts);

private:
  // Helpers called by TrackUpdate.
  void TrackWrite(uint64_t setKey,
                  const VkWriteDescriptorSet& write,
                  const std::vector<uint64_t>& handleKeys,
                  uint32_t& keyIdx);
  void TrackCopy(const VkCopyDescriptorSet& copy,
                 const std::vector<uint64_t>& copyHandleKeys,
                 uint32_t& keyIdx);

  // setKey -> (binding -> DescriptorBindingData)
  std::unordered_map<uint64_t, std::map<uint32_t, DescriptorBindingData>> m_Updates;

  // templateKey -> template entries (stored at vkCreateDescriptorUpdateTemplate time)
  std::unordered_map<uint64_t, std::vector<VkDescriptorUpdateTemplateEntry>> m_TemplateEntries;
};

} // namespace vulkan
} // namespace gits
