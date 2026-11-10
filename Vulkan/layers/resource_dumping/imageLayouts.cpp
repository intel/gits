// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imageLayouts.h"
#include "formatBlockInfo.h"

#include <algorithm>

namespace gits {
namespace vulkan {

ImageLayouts::ImageLayouts(
    VkFormat format, bool disjoint, uint32_t mipLevels, uint32_t arrayLayers, VkImageLayout layout)
    : m_MipLevels(mipLevels), m_ArrayLayers(arrayLayers) {
  const FormatBlockInfo info = GetFormatBlockInfo(format);
  if (info.IsDepthStencil && info.DepthBytes > 0 && info.StencilBytes > 0) {
    m_SlotAspects = {VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_ASPECT_STENCIL_BIT};
    m_AspectSlots = 2;
  } else if (info.IsMultiPlanar && disjoint) {
    m_AspectSlots =
        std::min<uint32_t>(info.PlaneCount, static_cast<uint32_t>(m_SlotAspects.size()));
    for (uint32_t plane = 0; plane < m_AspectSlots; ++plane) {
      m_SlotAspects[plane] = VK_IMAGE_ASPECT_PLANE_0_BIT << plane;
    }
  } else {
    m_SlotAspects[0] = AspectMaskForFormat(format, disjoint);
    m_AspectSlots = 1;
  }
  m_Layouts.assign(static_cast<size_t>(m_AspectSlots) * m_MipLevels * m_ArrayLayers, layout);
}

uint32_t ImageLayouts::SlotOf(VkImageAspectFlags aspect) const {
  for (uint32_t slot = 0; slot < m_AspectSlots; ++slot) {
    if (m_SlotAspects[slot] & aspect) {
      return slot;
    }
  }
  return 0;
}

void ImageLayouts::Set(const VkImageSubresourceRange& range, VkImageLayout layout) {
  if (range.baseMipLevel >= m_MipLevels || range.baseArrayLayer >= m_ArrayLayers) {
    return;
  }
  // A barrier on a disjoint image may name the color aspect for all of its planes.
  VkImageAspectFlags aspects = range.aspectMask;
  if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
    aspects |=
        VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT;
  }
  const uint32_t levelCount = range.levelCount == VK_REMAINING_MIP_LEVELS
                                  ? m_MipLevels - range.baseMipLevel
                                  : std::min(range.levelCount, m_MipLevels - range.baseMipLevel);
  const uint32_t layerCount =
      range.layerCount == VK_REMAINING_ARRAY_LAYERS
          ? m_ArrayLayers - range.baseArrayLayer
          : std::min(range.layerCount, m_ArrayLayers - range.baseArrayLayer);
  for (uint32_t slot = 0; slot < m_AspectSlots; ++slot) {
    if (m_AspectSlots > 1 && !(aspects & m_SlotAspects[slot])) {
      continue;
    }
    for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + levelCount; ++mip) {
      for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + layerCount;
           ++layer) {
        m_Layouts[Index(slot, mip, layer)] = layout;
      }
    }
  }
}

VkImageLayout ImageLayouts::Get(uint32_t aspectSlot, uint32_t mipLevel, uint32_t arrayLayer) const {
  if (aspectSlot >= m_AspectSlots || mipLevel >= m_MipLevels || arrayLayer >= m_ArrayLayers) {
    return VK_IMAGE_LAYOUT_UNDEFINED;
  }
  return m_Layouts[Index(aspectSlot, mipLevel, arrayLayer)];
}

std::vector<VkImageSubresourceRange> RenderedLayers(VkImageSubresourceRange viewRange,
                                                    uint32_t imageArrayLayers,
                                                    uint32_t layerCount,
                                                    uint32_t viewMask) {
  const uint32_t baseLayer = viewRange.baseArrayLayer;
  const uint32_t viewLayers = baseLayer >= imageArrayLayers ? 0
                              : viewRange.layerCount == VK_REMAINING_ARRAY_LAYERS
                                  ? imageArrayLayers - baseLayer
                                  : std::min(viewRange.layerCount, imageArrayLayers - baseLayer);
  std::vector<VkImageSubresourceRange> ranges;
  if (!viewMask) {
    viewRange.layerCount = std::min(viewLayers, layerCount);
    if (viewRange.layerCount) {
      ranges.push_back(viewRange);
    }
    return ranges;
  }
  constexpr uint32_t maskBits = 32;
  for (uint32_t view = 0; view < viewLayers && view < maskBits;) {
    if (!(viewMask >> view & 1)) {
      ++view;
      continue;
    }
    uint32_t end = view + 1;
    while (end < viewLayers && end < maskBits && (viewMask >> end & 1)) {
      ++end;
    }
    viewRange.baseArrayLayer = baseLayer + view;
    viewRange.layerCount = end - view;
    ranges.push_back(viewRange);
    view = end;
  }
  return ranges;
}

} // namespace vulkan
} // namespace gits
