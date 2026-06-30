// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorSetUpdateService.h"
#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "log.h"
#include <cstring>
#include <list>

namespace gits {
namespace vulkan {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Walk a Vulkan pNext chain and return the first node whose sType matches.
static const void* FindPNextStructure(const void* pNext, VkStructureType target) {
  const auto* node = static_cast<const VkBaseInStructure*>(pNext);
  while (node) {
    if (node->sType == target) {
      return node;
    }
    node = node->pNext;
  }
  return nullptr;
}

static bool IsImageDescriptorType(VkDescriptorType t) {
  return t == VK_DESCRIPTOR_TYPE_SAMPLER || t == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
         t == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || t == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
         t == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
}

static bool IsBufferDescriptorType(VkDescriptorType t) {
  return t == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || t == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
         t == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
         t == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

static bool IsTexelBufferDescriptorType(VkDescriptorType t) {
  return t == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
         t == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
}

// ---------------------------------------------------------------------------
// TrackWrite
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::TrackWrite(uint64_t setKey,
                                            const VkWriteDescriptorSet& write,
                                            const std::vector<uint64_t>& handleKeys,
                                            uint32_t& keyIdx) {
  // Key[0] is the dstSet key (already consumed by the caller to derive setKey).
  // keyIdx on entry already points past the dstSet key.

  auto& bindingData = m_Updates[setKey][write.dstBinding];
  bindingData.type = write.descriptorType;

  if (write.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
    // For inline uniform blocks, descriptorCount is the byte count and
    // dstArrayElement is the byte offset.  The actual data is in pNext.
    const auto* iub = static_cast<const VkWriteDescriptorSetInlineUniformBlock*>(FindPNextStructure(
        write.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK));
    if (iub && iub->pData && iub->dataSize > 0) {
      const uint32_t endByte = write.dstArrayElement + iub->dataSize;
      if (bindingData.inlineUniformData.size() < endByte) {
        bindingData.inlineUniformData.resize(endByte, 0);
      }
      std::memcpy(bindingData.inlineUniformData.data() + write.dstArrayElement, iub->pData,
                  iub->dataSize);
    }
    return;
  }

  // Grow the elements vector if needed.
  const uint32_t lastElem = write.dstArrayElement + write.descriptorCount;
  if (bindingData.elements.size() < lastElem) {
    bindingData.elements.resize(lastElem);
  }

  for (uint32_t i = 0; i < write.descriptorCount; ++i) {
    const uint32_t elem = write.dstArrayElement + i;
    auto& ed = bindingData.elements[elem];
    ed.descriptorType = write.descriptorType;

    if (IsImageDescriptorType(write.descriptorType)) {
      // Key layout: [samplerKey?] [imageViewKey?] per element.
      if ((write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
           write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
          keyIdx < handleKeys.size()) {
        ed.samplerKey = handleKeys[keyIdx++];
      }
      if (write.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER && keyIdx < handleKeys.size()) {
        ed.imageViewKey = handleKeys[keyIdx++];
      }
      if (write.pImageInfo) {
        ed.imageLayout = write.pImageInfo[i].imageLayout;
      }
    } else if (IsBufferDescriptorType(write.descriptorType)) {
      if (keyIdx < handleKeys.size()) {
        ed.bufferKey = handleKeys[keyIdx++];
      }
      if (write.pBufferInfo) {
        ed.bufferOffset = write.pBufferInfo[i].offset;
        ed.bufferRange = write.pBufferInfo[i].range;
      }
    } else if (IsTexelBufferDescriptorType(write.descriptorType)) {
      if (keyIdx < handleKeys.size()) {
        ed.bufferViewKey = handleKeys[keyIdx++];
      }
    } else if (write.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
      // AS keys are in the pNext-chain HandleKeys block (after the regular keys).
      // They are appended after all per-write image/buffer keys by
      // CollectHandleKeysInPNextChain, so we consume them here.
      if (keyIdx < handleKeys.size()) {
        ed.accelerationStructureKey = handleKeys[keyIdx++];
      }
    }
  }
}

// ---------------------------------------------------------------------------
// TrackCopy
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::TrackCopy(const VkCopyDescriptorSet& copy,
                                           const std::vector<uint64_t>& copyHandleKeys,
                                           uint32_t& keyIdx) {
  // Key layout: [srcSetKey] [dstSetKey] per VkCopyDescriptorSet.
  uint64_t srcSetKey = (keyIdx < copyHandleKeys.size()) ? copyHandleKeys[keyIdx++] : 0;
  uint64_t dstSetKey = (keyIdx < copyHandleKeys.size()) ? copyHandleKeys[keyIdx++] : 0;

  if (srcSetKey == 0 || dstSetKey == 0) {
    return;
  }

  auto srcSetIt = m_Updates.find(srcSetKey);
  if (srcSetIt == m_Updates.end()) {
    return;
  }
  auto srcBindIt = srcSetIt->second.find(copy.srcBinding);
  if (srcBindIt == srcSetIt->second.end()) {
    return;
  }

  const auto& srcBind = srcBindIt->second;
  auto& dstBind = m_Updates[dstSetKey][copy.dstBinding];
  dstBind.type = srcBind.type;

  if (srcBind.type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
    // For inline uniform: copy byte range [srcArrayElement, srcArrayElement+descriptorCount).
    const uint32_t srcBegin = copy.srcArrayElement;
    const uint32_t srcEnd = srcBegin + copy.descriptorCount;
    const uint32_t dstBegin = copy.dstArrayElement;
    const uint32_t dstEnd = dstBegin + copy.descriptorCount;

    if (srcEnd <= srcBind.inlineUniformData.size()) {
      if (dstBind.inlineUniformData.size() < dstEnd) {
        dstBind.inlineUniformData.resize(dstEnd, 0);
      }
      std::memcpy(dstBind.inlineUniformData.data() + dstBegin,
                  srcBind.inlineUniformData.data() + srcBegin, copy.descriptorCount);
    }
    return;
  }

  const uint32_t srcEnd = copy.srcArrayElement + copy.descriptorCount;
  const uint32_t dstEnd = copy.dstArrayElement + copy.descriptorCount;
  if (srcEnd > srcBind.elements.size()) {
    return;
  }
  if (dstBind.elements.size() < dstEnd) {
    dstBind.elements.resize(dstEnd);
  }
  for (uint32_t i = 0; i < copy.descriptorCount; ++i) {
    dstBind.elements[copy.dstArrayElement + i] = srcBind.elements[copy.srcArrayElement + i];
  }
}

// ---------------------------------------------------------------------------
// TrackUpdate
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::TrackUpdate(uint32_t descriptorWriteCount,
                                             const VkWriteDescriptorSet* pDescriptorWrites,
                                             const std::vector<uint64_t>& writeHandleKeys,
                                             uint32_t descriptorCopyCount,
                                             const VkCopyDescriptorSet* pDescriptorCopies,
                                             const std::vector<uint64_t>& copyHandleKeys) {
  uint32_t writeKeyIdx = 0;
  for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
    const auto& write = pDescriptorWrites[i];
    // First key per write: dstSet key.
    uint64_t setKey = (writeKeyIdx < writeHandleKeys.size()) ? writeHandleKeys[writeKeyIdx++] : 0;
    if (setKey == 0) {
      continue;
    }
    TrackWrite(setKey, write, writeHandleKeys, writeKeyIdx);
  }

  uint32_t copyKeyIdx = 0;
  for (uint32_t i = 0; i < descriptorCopyCount; ++i) {
    TrackCopy(pDescriptorCopies[i], copyHandleKeys, copyKeyIdx);
  }
}

// ---------------------------------------------------------------------------
// StoreTemplateEntries / RemoveTemplateEntries
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::StoreTemplateEntries(
    uint64_t templateKey, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo) {
  if (!pCreateInfo || templateKey == 0) {
    return;
  }
  m_TemplateEntries[templateKey].assign(pCreateInfo->pDescriptorUpdateEntries,
                                        pCreateInfo->pDescriptorUpdateEntries +
                                            pCreateInfo->descriptorUpdateEntryCount);
}

void DescriptorSetUpdateService::RemoveTemplateEntries(uint64_t templateKey) {
  m_TemplateEntries.erase(templateKey);
}

// ---------------------------------------------------------------------------
// TrackTemplateUpdate
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::TrackTemplateUpdate(uint64_t setKey,
                                                     uint64_t templateKey,
                                                     const std::vector<char>& pDataBytes) {
  auto it = m_TemplateEntries.find(templateKey);
  if (it == m_TemplateEntries.end()) {
    return;
  }

  for (const auto& entry : it->second) {
    if (entry.descriptorCount == 0) {
      continue;
    }

    auto& bindingData = m_Updates[setKey][entry.dstBinding];
    bindingData.type = entry.descriptorType;

    const uint32_t lastElem = entry.dstArrayElement + entry.descriptorCount;
    if (bindingData.elements.size() < lastElem) {
      bindingData.elements.resize(lastElem);
    }

    for (uint32_t i = 0; i < entry.descriptorCount; ++i) {
      const uint32_t elem = entry.dstArrayElement + i;
      auto& ed = bindingData.elements[elem];
      ed.descriptorType = entry.descriptorType;

      const size_t base = entry.offset + static_cast<size_t>(i) * entry.stride;

      if (IsImageDescriptorType(entry.descriptorType)) {
        if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
            entry.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          const size_t off = base + offsetof(VkDescriptorImageInfo, sampler);
          if (off + sizeof(uint64_t) <= pDataBytes.size()) {
            std::memcpy(&ed.samplerKey, pDataBytes.data() + off, sizeof(uint64_t));
          }
        }
        if (entry.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
          const size_t off = base + offsetof(VkDescriptorImageInfo, imageView);
          if (off + sizeof(uint64_t) <= pDataBytes.size()) {
            std::memcpy(&ed.imageViewKey, pDataBytes.data() + off, sizeof(uint64_t));
          }
        }
        const size_t layoutOff = base + offsetof(VkDescriptorImageInfo, imageLayout);
        if (layoutOff + sizeof(VkImageLayout) <= pDataBytes.size()) {
          std::memcpy(&ed.imageLayout, pDataBytes.data() + layoutOff, sizeof(VkImageLayout));
        }
      } else if (IsBufferDescriptorType(entry.descriptorType)) {
        const size_t bufOff = base + offsetof(VkDescriptorBufferInfo, buffer);
        if (bufOff + sizeof(uint64_t) <= pDataBytes.size()) {
          std::memcpy(&ed.bufferKey, pDataBytes.data() + bufOff, sizeof(uint64_t));
        }
        const size_t offOff = base + offsetof(VkDescriptorBufferInfo, offset);
        if (offOff + sizeof(VkDeviceSize) <= pDataBytes.size()) {
          std::memcpy(&ed.bufferOffset, pDataBytes.data() + offOff, sizeof(VkDeviceSize));
        }
        const size_t rangeOff = base + offsetof(VkDescriptorBufferInfo, range);
        if (rangeOff + sizeof(VkDeviceSize) <= pDataBytes.size()) {
          std::memcpy(&ed.bufferRange, pDataBytes.data() + rangeOff, sizeof(VkDeviceSize));
        }
      } else if (IsTexelBufferDescriptorType(entry.descriptorType)) {
        if (base + sizeof(uint64_t) <= pDataBytes.size()) {
          std::memcpy(&ed.bufferViewKey, pDataBytes.data() + base, sizeof(uint64_t));
        }
      }
      // VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: not currently supported
      // via templates in subcapture state tracking.
    }
  }
}

// ---------------------------------------------------------------------------
// RemoveDescriptorSet
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::RemoveDescriptorSet(uint64_t setKey) {
  m_Updates.erase(setKey);
}

// ---------------------------------------------------------------------------
// RestoreUpdates
// ---------------------------------------------------------------------------

void DescriptorSetUpdateService::RestoreUpdates(uint64_t setKey,
                                                SubcaptureRecorder& recorder,
                                                StateTrackingService& sts) {
  // ---- Regular vkUpdateDescriptorSets (emitted FIRST) ----
  //
  // Ordering rationale: the most common Vulkan pattern is:
  //   (a) initial setup via vkUpdateDescriptorSets (textures, samplers)
  //   (b) per-frame updates via vkUpdateDescriptorSetWithTemplate (UBOs)
  // Template writes are the FINAL state for their bindings in this pattern.
  // Emitting regular writes first and template writes last ensures template
  // values win, matching the old Vulkan RestoreDescriptorSetsUpdates approach
  // where all writes are merged into one element-level state map and only the
  // final merged state is emitted.
  auto updIt = m_Updates.find(setKey);
  if (updIt == m_Updates.end()) {
    return;
  }

  // Lifetime-storage for the per-write auxiliary data (pImageInfo, pBufferInfo,
  // pTexelBufferView, inline-block structs) that the serializer will reference.
  // These must outlive the Record() call.
  std::list<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
  std::list<std::vector<VkDescriptorBufferInfo>> bufferInfoStorage;
  std::list<std::vector<VkBufferView>> texelViewStorage;
  std::list<VkWriteDescriptorSetInlineUniformBlock> iubStorage;
  std::list<std::vector<VkAccelerationStructureKHR>> asStorage;
  std::list<VkWriteDescriptorSetAccelerationStructureKHR> asWriteStorage;

  std::vector<VkWriteDescriptorSet> writes;
  std::vector<uint64_t> writeHandleKeys;

  for (auto& [binding, bindData] : updIt->second) {
    if (bindData.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) {
      continue;
    }

    if (bindData.type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      if (bindData.inlineUniformData.empty()) {
        continue;
      }
      // Inline uniform block: emit one write for the whole block.
      iubStorage.push_back(VkWriteDescriptorSetInlineUniformBlock{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK, // sType
          nullptr,                                                     // pNext
          static_cast<uint32_t>(bindData.inlineUniformData.size()),    // dataSize
          bindData.inlineUniformData.data()                            // pData
      });
      VkWriteDescriptorSet w{};
      w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      w.pNext = &iubStorage.back();
      w.dstSet = VK_NULL_HANDLE; // resolved from HandleKeys
      w.dstBinding = binding;
      w.dstArrayElement = 0;
      w.descriptorCount = static_cast<uint32_t>(bindData.inlineUniformData.size());
      w.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;

      writeHandleKeys.push_back(setKey); // dstSet key
      writes.push_back(w);
      continue;
    }

    // Non-inline types: iterate over array elements, emitting one write per
    // contiguous run of valid (explicitly written) elements.
    uint32_t runStart = UINT32_MAX;
    // We'll flush each run when we encounter an invalid element or the end.
    auto flushRun = [&](uint32_t runEnd) {
      if (runStart == UINT32_MAX || runEnd == runStart) {
        return;
      }
      const uint32_t count = runEnd - runStart;
      VkWriteDescriptorSet w{};
      w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      w.dstSet = VK_NULL_HANDLE;
      w.dstBinding = binding;
      w.dstArrayElement = runStart;
      w.descriptorCount = count;
      w.descriptorType = bindData.type;

      writeHandleKeys.push_back(setKey); // dstSet key

      if (IsImageDescriptorType(bindData.type)) {
        imageInfoStorage.push_back(std::vector<VkDescriptorImageInfo>(count));
        auto& infos = imageInfoStorage.back();
        for (uint32_t i = 0; i < count; ++i) {
          const auto& ed = bindData.elements[runStart + i];
          infos[i].sampler = VK_NULL_HANDLE;
          infos[i].imageView = VK_NULL_HANDLE;
          infos[i].imageLayout = ed.imageLayout;
          if (bindData.type == VK_DESCRIPTOR_TYPE_SAMPLER ||
              bindData.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            writeHandleKeys.push_back(ed.samplerKey);
          }
          if (bindData.type != VK_DESCRIPTOR_TYPE_SAMPLER) {
            writeHandleKeys.push_back(ed.imageViewKey);
          }
        }
        w.pImageInfo = infos.data();
      } else if (IsBufferDescriptorType(bindData.type)) {
        bufferInfoStorage.push_back(std::vector<VkDescriptorBufferInfo>(count));
        auto& infos = bufferInfoStorage.back();
        for (uint32_t i = 0; i < count; ++i) {
          const auto& ed = bindData.elements[runStart + i];
          infos[i].buffer = VK_NULL_HANDLE;
          infos[i].offset = ed.bufferOffset;
          infos[i].range = ed.bufferRange;
          writeHandleKeys.push_back(ed.bufferKey);
        }
        w.pBufferInfo = infos.data();
      } else if (IsTexelBufferDescriptorType(bindData.type)) {
        texelViewStorage.push_back(std::vector<VkBufferView>(count, VK_NULL_HANDLE));
        for (uint32_t i = 0; i < count; ++i) {
          writeHandleKeys.push_back(bindData.elements[runStart + i].bufferViewKey);
        }
        w.pTexelBufferView = texelViewStorage.back().data();
      } else if (bindData.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
        asStorage.push_back(std::vector<VkAccelerationStructureKHR>(count, VK_NULL_HANDLE));
        asWriteStorage.push_back(VkWriteDescriptorSetAccelerationStructureKHR{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, nullptr, count,
            asStorage.back().data()});
        w.pNext = &asWriteStorage.back();
        for (uint32_t i = 0; i < count; ++i) {
          writeHandleKeys.push_back(bindData.elements[runStart + i].accelerationStructureKey);
        }
      }

      writes.push_back(w);
      runStart = UINT32_MAX;
    };

    for (uint32_t e = 0; e < static_cast<uint32_t>(bindData.elements.size()); ++e) {
      const auto& ed = bindData.elements[e];
      if (ed.descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM) {
        flushRun(e);
        continue;
      }
      // Resource-validity checks: use IsRestored() rather than HasState() so
      // that we skip bindings whose resource exists in state but failed to
      // restore (e.g. an image view whose backing image was destroyed before
      // the subcapture point).  This mirrors the old Vulkan
      // RestoreDescriptorSetsUpdates helpers which walk the full object chain
      // (sampler ? imageView ? image) and skip if any link is missing.
      bool valid = true;
      if (IsImageDescriptorType(ed.descriptorType)) {
        if ((ed.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
             ed.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
            ed.samplerKey != 0 && !sts.IsRestored(ed.samplerKey)) {
          LOG_INFO << "Vulkan2 subcapture: omitting descriptor write for set key=" << setKey
                   << " binding=" << binding << " element=" << e
                   << " because VkSampler key=" << ed.samplerKey << " was not restored.";
          valid = false;
        }
        if (valid && ed.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER && ed.imageViewKey != 0 &&
            !sts.IsRestored(ed.imageViewKey)) {
          LOG_INFO << "Vulkan2 subcapture: omitting descriptor write for set key=" << setKey
                   << " binding=" << binding << " element=" << e
                   << " because VkImageView key=" << ed.imageViewKey << " was not restored.";
          valid = false;
        }
      } else if (IsBufferDescriptorType(ed.descriptorType)) {
        if (ed.bufferKey != 0 && !sts.IsRestored(ed.bufferKey)) {
          LOG_INFO << "Vulkan2 subcapture: omitting descriptor write for set key=" << setKey
                   << " binding=" << binding << " element=" << e
                   << " because VkBuffer key=" << ed.bufferKey << " was not restored.";
          valid = false;
        }
      } else if (IsTexelBufferDescriptorType(ed.descriptorType)) {
        if (ed.bufferViewKey != 0 && !sts.IsRestored(ed.bufferViewKey)) {
          LOG_INFO << "Vulkan2 subcapture: omitting descriptor write for set key=" << setKey
                   << " binding=" << binding << " element=" << e
                   << " because VkBufferView key=" << ed.bufferViewKey << " was not restored.";
          valid = false;
        }
      } else if (ed.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
        if (ed.accelerationStructureKey != 0 && !sts.IsRestored(ed.accelerationStructureKey)) {
          LOG_INFO << "Vulkan2 subcapture: omitting descriptor write for set key=" << setKey
                   << " binding=" << binding << " element=" << e
                   << " because VkAccelerationStructureKHR key=" << ed.accelerationStructureKey
                   << " was not restored.";
          valid = false;
        }
      }

      if (!valid) {
        flushRun(e);
        continue;
      }
      if (runStart == UINT32_MAX) {
        runStart = e;
      }
    }
    flushRun(static_cast<uint32_t>(bindData.elements.size()));
  }

  if (!writes.empty()) {
    // Resolve device key via DescriptorSetState::parentKey (device key is set
    // directly - see SubcaptureLayer::Post(vkAllocateDescriptorSetsCommand)).
    // If the descriptor set state is gone (freed before subcapture), skip:
    // its key won't be in HandleMapService and playback would assert.
    ObjectState* dsState = sts.GetState(setKey);
    if (!dsState) {
      LOG_INFO << "Vulkan2 subcapture: skipping descriptor update restore for set key=" << setKey
               << " because the descriptor set state no longer exists.";
      return;
    }
    uint64_t deviceKey = dsState->parentKey;

    vkUpdateDescriptorSetsCommand cmd;
    cmd.m_device.Key = deviceKey;
    cmd.m_descriptorWriteCount.Value = static_cast<uint32_t>(writes.size());
    cmd.m_pDescriptorWrites.Value = writes.data();
    cmd.m_pDescriptorWrites.Size = static_cast<uint32_t>(writes.size());
    cmd.m_pDescriptorWrites.HandleKeys = writeHandleKeys;
    cmd.m_descriptorCopyCount.Value = 0;
    cmd.m_pDescriptorCopies.Value = nullptr;
    cmd.m_pDescriptorCopies.Size = 0;

    recorder.Record(vkUpdateDescriptorSetsSerializer(cmd));
  }
}

} // namespace vulkan
} // namespace gits
