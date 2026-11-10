// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "formatBlockInfo.h"

#include <algorithm>

namespace gits {
namespace vulkan {

FormatBlockInfo GetFormatBlockInfo(VkFormat fmt) {
  switch (fmt) {
  // ---- Single-channel ----
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8_SRGB:
    return {1, 1, 1};
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16_SFLOAT:
    return {1, 1, 2};
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32_SFLOAT:
    return {1, 1, 4};
  // ---- Two-channel ----
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8_SRGB:
    return {1, 1, 2};
  case VK_FORMAT_R16G16_SFLOAT:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16_SINT:
    return {1, 1, 4};
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32_SINT:
    return {1, 1, 8};
  // ---- Four-channel (most common) ----
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8G8B8A8_SRGB:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    return {1, 1, 4};
  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return {1, 1, 8};
  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return {1, 1, 16};
  // ---- BC compressed ----
  case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
  case VK_FORMAT_BC4_UNORM_BLOCK:
  case VK_FORMAT_BC4_SNORM_BLOCK:
    return {4, 4, 8};
  case VK_FORMAT_BC2_UNORM_BLOCK:
  case VK_FORMAT_BC2_SRGB_BLOCK:
  case VK_FORMAT_BC3_UNORM_BLOCK:
  case VK_FORMAT_BC3_SRGB_BLOCK:
  case VK_FORMAT_BC5_UNORM_BLOCK:
  case VK_FORMAT_BC5_SNORM_BLOCK:
  case VK_FORMAT_BC6H_UFLOAT_BLOCK:
  case VK_FORMAT_BC6H_SFLOAT_BLOCK:
  case VK_FORMAT_BC7_UNORM_BLOCK:
  case VK_FORMAT_BC7_SRGB_BLOCK:
    return {4, 4, 16};
  // ---- YCbCr packed single-plane ----
  // 4 bytes cover a 2-wide texel pair (macropixel).
  case VK_FORMAT_G8B8G8R8_422_UNORM:
  case VK_FORMAT_B8G8R8G8_422_UNORM:
    return {2, 1, 4};
  // ---- YCbCr multi-planar ----
  // 2-plane 4:2:0 (NV12 / P010 / P012 / P016):
  //   plane 0 = luma  (Y),    full resolution, bytesPerPixel=1 (8b) or 2 (10/12/16b)
  //   plane 1 = chroma (CbCr), half width+height, bytesPerPixel=2 or 4
  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 4};
    return fi;
  }
  // 3-plane 4:2:0 (I420 / YV12 variants):
  //   plane 0 = Y (full), plane 1 = Cb (quarter area), plane 2 = Cr (quarter area)
  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 1};
    fi.Planes[2] = {2, 2, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 2};
    fi.Planes[2] = {2, 2, 2};
    return fi;
  }
  // 2-plane 4:2:2: chroma is half-width but full-height
  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 4};
    return fi;
  }
  // 3-plane 4:2:2: chroma is half-width, full-height, per-channel
  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 1};
    fi.Planes[2] = {2, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 2};
    fi.Planes[2] = {2, 1, 2};
    return fi;
  }
  // 3-plane 4:4:4: no chroma subsampling, all planes full resolution
  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {1, 1, 1};
    fi.Planes[2] = {1, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {1, 1, 2};
    fi.Planes[2] = {1, 1, 2};
    return fi;
  }
  // ---- Depth / stencil ----
  case VK_FORMAT_D16_UNORM:
    return {1, 1, 0, true, 2, 0};
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D32_SFLOAT:
    return {1, 1, 0, true, 4, 0};
  case VK_FORMAT_S8_UINT:
    return {1, 1, 0, true, 0, 1};
  case VK_FORMAT_D16_UNORM_S8_UINT:
    return {1, 1, 0, true, 2, 1};
  case VK_FORMAT_D24_UNORM_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  default: {
    FormatBlockInfo fi{};
    fi.Known = false;
    return fi;
  }
  }
}

VkDeviceSize ComputeImageStagingLayout(VkFormat format,
                                       const VkExtent3D& extent,
                                       uint32_t mipLevels,
                                       uint32_t arrayLayers,
                                       std::vector<VkBufferImageCopy>& outRegions) {
  const auto fi = GetFormatBlockInfo(format);
  VkDeviceSize offset = 0;

  auto addRegion = [&](VkImageAspectFlags aspect, uint32_t layer, uint32_t mip, uint32_t w,
                       uint32_t h, uint32_t d, uint32_t bw, uint32_t bh, uint32_t bytesPerBlock) {
    // Buffer offsets must be aligned to at least 4 bytes (spec requirement).
    const VkDeviceSize align = std::max(4u, bytesPerBlock);
    offset = (offset + align - 1) & ~(align - 1);

    VkBufferImageCopy region{};
    region.bufferOffset = offset;
    region.bufferRowLength = 0; // tight packing
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = aspect;
    region.imageSubresource.mipLevel = mip;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {w, h, d};
    outRegions.push_back(region);

    uint32_t blocksX = (w + bw - 1) / bw;
    uint32_t blocksY = (h + bh - 1) / bh;
    offset += static_cast<VkDeviceSize>(blocksX) * blocksY * d * bytesPerBlock;
  };

  for (uint32_t layer = 0; layer < arrayLayers; ++layer) {
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
      uint32_t w = std::max(1u, extent.width >> mip);
      uint32_t h = std::max(1u, extent.height >> mip);
      uint32_t d = std::max(1u, extent.depth >> mip);

      if (fi.IsMultiPlanar) {
        static constexpr VkImageAspectFlags kPlaneAspect[3] = {
            VK_IMAGE_ASPECT_PLANE_0_BIT,
            VK_IMAGE_ASPECT_PLANE_1_BIT,
            VK_IMAGE_ASPECT_PLANE_2_BIT,
        };
        for (uint8_t p = 0; p < fi.PlaneCount; ++p) {
          uint32_t pw = std::max(1u, w / fi.Planes[p].WidthDivisor);
          uint32_t ph = std::max(1u, h / fi.Planes[p].HeightDivisor);
          addRegion(kPlaneAspect[p], layer, mip, pw, ph, d, 1, 1, fi.Planes[p].BytesPerPixel);
        }
      } else if (!fi.IsDepthStencil) {
        addRegion(VK_IMAGE_ASPECT_COLOR_BIT, layer, mip, w, h, d, fi.BlockWidth, fi.BlockHeight,
                  fi.BytesPerBlock);
      } else {
        if (fi.DepthBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_DEPTH_BIT, layer, mip, w, h, d, 1, 1, fi.DepthBytes);
        }
        if (fi.StencilBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_STENCIL_BIT, layer, mip, w, h, d, 1, 1, fi.StencilBytes);
        }
      }
    }
  }
  return offset;
}

VkDeviceSize StagingOffsetAlignment(VkFormat format) {
  const auto fi = GetFormatBlockInfo(format);
  uint32_t maxUnit = fi.BytesPerBlock;
  if (fi.IsMultiPlanar) {
    for (uint8_t p = 0; p < fi.PlaneCount; ++p) {
      maxUnit = std::max(maxUnit, fi.Planes[p].BytesPerPixel);
    }
  } else if (fi.IsDepthStencil) {
    maxUnit = std::max(fi.DepthBytes, fi.StencilBytes);
  }
  return std::max(4u, maxUnit);
}

VkImageAspectFlags AspectMaskForFormat(VkFormat format, bool disjoint) {
  const auto fi = GetFormatBlockInfo(format);
  if (fi.IsDepthStencil) {
    VkImageAspectFlags aspect = 0;
    if (fi.DepthBytes > 0) {
      aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (fi.StencilBytes > 0) {
      aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return aspect;
  }
  // A barrier on a disjoint multi-planar image must name its plane(s)
  // (VUID-VkImageMemoryBarrier-image-01672 forbids COLOR there); a
  // non-disjoint one must use COLOR instead (VUID-VkImageMemoryBarrier-image-
  // 09242 forbids plane bits there).  Copy regions are unaffected by this:
  // each one already names a single plane bit, built separately from this
  // mask (VUID-vkCmdCopyBufferToImage-dstImage-07981).
  if (fi.IsMultiPlanar && disjoint) {
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;
    if (fi.PlaneCount >= 3) {
      aspect |= VK_IMAGE_ASPECT_PLANE_2_BIT;
    }
    return aspect;
  }
  return VK_IMAGE_ASPECT_COLOR_BIT;
}

} // namespace vulkan
} // namespace gits
