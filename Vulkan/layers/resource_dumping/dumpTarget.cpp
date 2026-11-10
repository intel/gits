// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dumpTarget.h"

#include <algorithm>
#include <numeric>

namespace gits {
namespace vulkan {

namespace {

bool InRange(uint32_t value, uint32_t base, uint32_t count) {
  return value >= base && (count == VK_REMAINING_MIP_LEVELS || value - base < count);
}

bool IsSelected(const DumpTarget& target,
                VkImageAspectFlags slotAspect,
                uint32_t mipLevel,
                uint32_t arrayLayer) {
  if (target.Selection.empty()) {
    return true;
  }
  for (const VkImageSubresourceRange& range : target.Selection) {
    VkImageAspectFlags aspects = range.aspectMask;
    if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      aspects |=
          VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT;
    }
    if ((aspects & slotAspect) && InRange(mipLevel, range.baseMipLevel, range.levelCount) &&
        InRange(arrayLayer, range.baseArrayLayer, range.layerCount)) {
      return true;
    }
  }
  return false;
}

bool IsCopiedInPlace(VkImageLayout layout) {
  return layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || layout == VK_IMAGE_LAYOUT_GENERAL ||
         layout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
}

void AddTransition(std::vector<VkImageMemoryBarrier>& barriers,
                   VkImage image,
                   VkImageAspectFlags aspect,
                   VkImageLayout layout,
                   uint32_t mipLevel,
                   uint32_t arrayLayer) {
  if (!IsContentDefined(layout) || IsCopiedInPlace(layout)) {
    return;
  }
  if (!barriers.empty()) {
    VkImageMemoryBarrier& last = barriers.back();
    VkImageSubresourceRange& range = last.subresourceRange;
    if (last.image == image && last.oldLayout == layout && range.aspectMask == aspect &&
        range.baseMipLevel == mipLevel && range.baseArrayLayer + range.layerCount == arrayLayer) {
      ++range.layerCount;
      return;
    }
  }
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.oldLayout = layout;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange = {aspect, mipLevel, 1, arrayLayer, 1};
  barriers.push_back(barrier);
}

} // namespace

bool IsContentDefined(VkImageLayout layout) {
  return layout != VK_IMAGE_LAYOUT_UNDEFINED && layout != VK_IMAGE_LAYOUT_PREINITIALIZED;
}

bool IsCopied(const DumpTarget& target,
              uint32_t aspectSlot,
              uint32_t mipLevel,
              uint32_t arrayLayer) {
  const ImageLayouts& layouts = target.Layouts;
  return !(layouts.SlotAspect(aspectSlot) & target.ExcludedAspects) &&
         IsContentDefined(layouts.Get(aspectSlot, mipLevel, arrayLayer)) &&
         IsSelected(target, layouts.SlotAspect(aspectSlot), mipLevel, arrayLayer);
}

std::vector<VkImageMemoryBarrier> TransferSrcBarriers(const std::vector<const DumpTarget*>& targets,
                                                      bool separateDepthStencilLayouts) {
  std::vector<VkImageMemoryBarrier> barriers;
  if (targets.empty()) {
    return barriers;
  }
  const ImageLayouts& layouts = targets.front()->Layouts;
  const VkImage image = targets.front()->Image;
  const auto copiedBy = [&targets](uint32_t slot, uint32_t mip,
                                   uint32_t layer) -> const DumpTarget* {
    for (const DumpTarget* target : targets) {
      if (IsCopied(*target, slot, mip, layer)) {
        return target;
      }
    }
    return nullptr;
  };
  const bool depthStencil =
      layouts.AspectSlots() == 2 && layouts.SlotAspect(0) == VK_IMAGE_ASPECT_DEPTH_BIT;
  for (uint32_t mip = 0; mip < layouts.MipLevels(); ++mip) {
    for (uint32_t layer = 0; layer < layouts.ArrayLayers(); ++layer) {
      VkImageAspectFlags aspects = 0;
      std::optional<uint32_t> firstCopied;
      for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
        aspects |= layouts.SlotAspect(slot);
        if (!firstCopied && copiedBy(slot, mip, layer)) {
          firstCopied = slot;
        }
      }
      if (!firstCopied) {
        continue;
      }
      // A barrier names several aspects only if all of them are copied from the same layout, or if
      // the depth and stencil layouts cannot be separate.
      const VkImageLayout layout =
          copiedBy(*firstCopied, mip, layer)->Layouts.Get(*firstCopied, mip, layer);
      bool allCopiedAlike = true;
      for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
        const DumpTarget* target = copiedBy(slot, mip, layer);
        allCopiedAlike =
            allCopiedAlike && target && target->Layouts.Get(slot, mip, layer) == layout;
      }
      if (allCopiedAlike || (depthStencil && !separateDepthStencilLayouts)) {
        AddTransition(barriers, image, aspects, layout, mip, layer);
        continue;
      }
      for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
        if (const DumpTarget* target = copiedBy(slot, mip, layer)) {
          AddTransition(barriers, image, layouts.SlotAspect(slot),
                        target->Layouts.Get(slot, mip, layer), mip, layer);
        }
      }
    }
  }
  return barriers;
}

std::vector<VkImageMemoryBarrier> TransferSrcBarriers(const DumpTarget& target,
                                                      bool separateDepthStencilLayouts) {
  return TransferSrcBarriers(std::vector<const DumpTarget*>{&target}, separateDepthStencilLayouts);
}

VkImageLayout CopySourceLayout(const DumpTarget& target,
                               const VkImageSubresourceLayers& subresource) {
  const ImageLayouts& layouts = target.Layouts;
  const VkImageLayout layout = layouts.Get(layouts.SlotOf(subresource.aspectMask),
                                           subresource.mipLevel, subresource.baseArrayLayer);
  return IsCopiedInPlace(layout) ? layout : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

VkDeviceSize CopyRegions(const DumpTarget& target, std::vector<VkBufferImageCopy>& regions) {
  const ImageLayouts& layouts = target.Layouts;
  std::vector<VkBufferImageCopy> all;
  ComputeImageStagingLayout(target.Format, target.Extent, layouts.MipLevels(),
                            layouts.ArrayLayers(), all);
  const FormatBlockInfo info = GetFormatBlockInfo(target.Format);
  VkDeviceSize offset = 0;
  for (VkBufferImageCopy region : all) {
    const VkImageSubresourceLayers& subresource = region.imageSubresource;
    if (!IsCopied(target, layouts.SlotOf(subresource.aspectMask), subresource.mipLevel,
                  subresource.baseArrayLayer)) {
      continue;
    }
    const uint32_t depth = region.imageExtent.depth;
    const uint32_t firstSlice = std::min(target.BaseSlice, depth);
    const uint32_t slices = target.SliceCount == VK_REMAINING_ARRAY_LAYERS
                                ? depth - firstSlice
                                : std::min(target.SliceCount, depth - firstSlice);
    if (slices == 0) {
      continue;
    }
    region.imageOffset.z = static_cast<int32_t>(firstSlice);
    region.imageExtent.depth = slices;

    // Buffer offsets must be multiples of the texel block size, and of 4 for depth/stencil.
    const RegionTexels texels = GetRegionTexels(info, subresource.aspectMask);
    const VkDeviceSize alignment =
        std::lcm<VkDeviceSize>(4, std::max<VkDeviceSize>(1, texels.BytesPerBlock));
    offset = (offset + alignment - 1) / alignment * alignment;
    region.bufferOffset = offset;
    const VkDeviceSize blocksX =
        (region.imageExtent.width + texels.BlockWidth - 1) / texels.BlockWidth;
    const VkDeviceSize blocksY =
        (region.imageExtent.height + texels.BlockHeight - 1) / texels.BlockHeight;
    offset += blocksX * blocksY * slices * texels.BytesPerBlock;
    regions.push_back(region);
  }
  return offset;
}

RegionTexels GetRegionTexels(const FormatBlockInfo& info, VkImageAspectFlags aspect) {
  switch (aspect) {
  case VK_IMAGE_ASPECT_DEPTH_BIT:
    return {1, 1, info.DepthBytes};
  case VK_IMAGE_ASPECT_STENCIL_BIT:
    return {1, 1, info.StencilBytes};
  case VK_IMAGE_ASPECT_PLANE_0_BIT:
    return {1, 1, info.Planes[0].BytesPerPixel};
  case VK_IMAGE_ASPECT_PLANE_1_BIT:
    return {1, 1, info.Planes[1].BytesPerPixel};
  case VK_IMAGE_ASPECT_PLANE_2_BIT:
    return {1, 1, info.Planes[2].BytesPerPixel};
  default:
    return {info.BlockWidth, info.BlockHeight, info.BytesPerBlock};
  }
}

std::optional<LayoutMismatch> FindLayoutMismatch(const DumpTarget& target,
                                                 const ImageLayouts& actual) {
  const ImageLayouts& layouts = target.Layouts;
  for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
    for (uint32_t mip = 0; mip < layouts.MipLevels(); ++mip) {
      for (uint32_t layer = 0; layer < layouts.ArrayLayers(); ++layer) {
        if (IsCopied(target, slot, mip, layer) &&
            actual.Get(slot, mip, layer) != layouts.Get(slot, mip, layer)) {
          return LayoutMismatch{layouts.Get(slot, mip, layer), actual.Get(slot, mip, layer)};
        }
      }
    }
  }
  return std::nullopt;
}

} // namespace vulkan
} // namespace gits
