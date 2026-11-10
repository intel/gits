// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imageLayouts.h"
#include "formatBlockInfo.h"
#include "enumsAuto.h"
#include "vulkanHeader2.h"

#include <filesystem>
#include <optional>
#include <vector>

namespace gits {
namespace vulkan {

// A buffer, or every subresource of an image whose layout is defined, to copy at a dump point.
struct DumpTarget {
  std::filesystem::path Name;
  VkBuffer Buffer{};
  VkDeviceSize BufferSize{};
  VkImage Image{};
  VkFormat Format{};
  VkExtent3D Extent{};
  bool Disjoint{};
  // The layouts of every subresource of the image at the dump point.
  ImageLayouts Layouts;
  // The aspects that are not copied, as the image was created without the transfer source usage
  // for them.
  VkImageAspectFlags ExcludedAspects{};
  // The file format of images that can be converted to 8 bit RGBA.
  ImageFormat FileFormat{ImageFormat::PNG};
};

bool IsContentDefined(VkImageLayout layout);

// Whether a subresource of the image is copied: its aspect is not excluded and its layout is
// defined.
bool IsCopied(const DumpTarget& target,
              uint32_t aspectSlot,
              uint32_t mipLevel,
              uint32_t arrayLayer);

// The barriers that move the copied subresources to the layouts the copies read them in. Without
// separateDepthStencilLayouts a barrier on a depth/stencil image names both aspects, so an aspect
// that is not copied is transitioned along with the copied one.
std::vector<VkImageMemoryBarrier> TransferSrcBarriers(const DumpTarget& target,
                                                      bool separateDepthStencilLayouts);

// The layout the copies read a subresource in. A subresource in VK_IMAGE_LAYOUT_GENERAL or
// VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR is copied in its layout: a shared presentable image must stay
// in its layout, and a layout transition is a write that work of other queues reading the image
// at the same time does not wait for. Other subresources are copied in
// VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
VkImageLayout CopySourceLayout(const DumpTarget& target,
                               const VkImageSubresourceLayers& subresource);

// Appends the tightly packed copy regions of the copied subresources and returns the staging bytes
// they need.
VkDeviceSize CopyRegions(const DumpTarget& target, std::vector<VkBufferImageCopy>& regions);

struct RegionTexels {
  uint32_t BlockWidth{1};
  uint32_t BlockHeight{1};
  uint32_t BytesPerBlock{};
};

RegionTexels GetRegionTexels(const FormatBlockInfo& info, VkImageAspectFlags aspect);

struct LayoutMismatch {
  VkImageLayout Expected{};
  VkImageLayout Actual{};
};

// The first copied subresource whose layout in actual differs from the one the copies of the target
// transition it from.
std::optional<LayoutMismatch> FindLayoutMismatch(const DumpTarget& target,
                                                 const ImageLayouts& actual);

} // namespace vulkan
} // namespace gits
