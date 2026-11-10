// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

struct FormatBlockInfo {
  uint32_t BlockWidth{1};
  uint32_t BlockHeight{1};
  uint32_t BytesPerBlock{4};
  bool IsDepthStencil{false};
  uint32_t DepthBytes{0};   // 0 = no depth
  uint32_t StencilBytes{0}; // 0 = no stencil
  // Multi-planar YCbCr formats: copy requires one region per plane with
  // VK_IMAGE_ASPECT_PLANE_*_BIT and per-plane extent/stride.
  bool IsMultiPlanar{false};
  uint8_t PlaneCount{0};
  struct PlaneDesc {
    uint32_t WidthDivisor{1};  // plane width  = image width  / WidthDivisor
    uint32_t HeightDivisor{1}; // plane height = image height / HeightDivisor
    uint32_t BytesPerPixel{1};
  } Planes[3]{};
  // False for a format missing from the table, whose layout then assumes 4 bytes per texel.
  bool Known{true};
};

FormatBlockInfo GetFormatBlockInfo(VkFormat format);

// Returns the staging bytes of a tightly packed copy of all subresources and appends one copy
// region per subresource and aspect (per plane for multi-planar formats) to outRegions.
VkDeviceSize ComputeImageStagingLayout(VkFormat format,
                                       const VkExtent3D& extent,
                                       uint32_t mipLevels,
                                       uint32_t arrayLayers,
                                       std::vector<VkBufferImageCopy>& outRegions);

// Basing an image's regions at a multiple of this keeps them all legally aligned.
VkDeviceSize StagingOffsetAlignment(VkFormat format);

// Aspect mask to use in image barriers and barrier subresource ranges for a
// format: DEPTH and/or STENCIL for depth/stencil formats; for color formats,
// COLOR unless the image is a disjoint multi-planar one (VK_IMAGE_CREATE_
// DISJOINT_BIT), which must use its plane bits instead
// (VUID-VkImageMemoryBarrier-image-01672 / -image-09242). The recorder's
// readback barriers and the player's upload barriers both go through this so
// they cannot disagree.  Not for copy regions - those need one plane bit per
// region instead.
VkImageAspectFlags AspectMaskForFormat(VkFormat format, bool disjoint = false);

} // namespace vulkan
} // namespace gits
