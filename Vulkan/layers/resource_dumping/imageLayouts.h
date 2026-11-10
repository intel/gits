// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <array>
#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

// Layout of every subresource of an image. Aspects whose layouts may differ are kept apart: the
// depth and stencil aspects of a depth/stencil format, as separateDepthStencilLayouts lets them
// differ, and the planes of a disjoint multi-planar image. Every other image has one layout for
// all of its aspects.
class ImageLayouts {
public:
  ImageLayouts() = default;
  ImageLayouts(VkFormat format,
               bool disjoint,
               uint32_t mipLevels,
               uint32_t arrayLayers,
               VkImageLayout layout);

  void Set(const VkImageSubresourceRange& range, VkImageLayout layout);
  VkImageLayout Get(uint32_t aspectSlot, uint32_t mipLevel, uint32_t arrayLayer) const;

  uint32_t AspectSlots() const {
    return m_AspectSlots;
  }
  uint32_t MipLevels() const {
    return m_MipLevels;
  }
  uint32_t ArrayLayers() const {
    return m_ArrayLayers;
  }
  // The aspect slot whose layout applies to a copy region of the given aspect.
  uint32_t SlotOf(VkImageAspectFlags aspect) const;
  // The aspects a barrier names for the subresources of the aspect slot.
  VkImageAspectFlags SlotAspect(uint32_t aspectSlot) const {
    return aspectSlot < m_AspectSlots ? m_SlotAspects[aspectSlot] : 0;
  }

private:
  size_t Index(uint32_t aspectSlot, uint32_t mipLevel, uint32_t arrayLayer) const {
    return (static_cast<size_t>(aspectSlot) * m_MipLevels + mipLevel) * m_ArrayLayers + arrayLayer;
  }

  std::array<VkImageAspectFlags, 3> m_SlotAspects{};
  uint32_t m_AspectSlots{};
  uint32_t m_MipLevels{};
  uint32_t m_ArrayLayers{};
  std::vector<VkImageLayout> m_Layouts;
};

// The array layers of an attachment view, of an image with imageArrayLayers layers, that a render
// pass instance renders: the layers of the views in viewMask, or without multiview the first
// layerCount layers.
std::vector<VkImageSubresourceRange> RenderedLayers(VkImageSubresourceRange viewRange,
                                                    uint32_t imageArrayLayers,
                                                    uint32_t layerCount,
                                                    uint32_t viewMask);

} // namespace vulkan
} // namespace gits
