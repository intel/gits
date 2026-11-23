// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "externalOwnership.h"

#include "objectState.h"

namespace gits {
namespace vulkan {

namespace {

ExternalOwnership GetExternalOwnership(uint32_t externalFamily,
                                       VkSharingMode sharingMode,
                                       uint32_t queueFamily) {
  ExternalOwnership ownership;
  ownership.ExternalFamily = externalFamily;
  ownership.QueueFamily =
      sharingMode == VK_SHARING_MODE_CONCURRENT ? VK_QUEUE_FAMILY_IGNORED : queueFamily;
  return ownership;
}

} // namespace

bool IsExternalQueueFamily(uint32_t family) {
  return family == VK_QUEUE_FAMILY_EXTERNAL || family == VK_QUEUE_FAMILY_FOREIGN_EXT;
}

bool IsReleasedToExternal(const ImageState& image) {
  if (!IsExternalQueueFamily(image.ExternalOwnerFamily) || image.ExternalOwnershipPartial) {
    return false;
  }
  if (image.SharingMode == VK_SHARING_MODE_CONCURRENT) {
    return true;
  }
  return image.ExclusiveOwnershipPending && !image.ExclusiveOwnershipMixed &&
         image.PartialOwnerStates.empty();
}

bool IsReleasedToExternal(const BufferState& buffer) {
  return IsExternalQueueFamily(buffer.ExternalOwnerFamily) && !buffer.ExternalOwnershipPartial;
}

ExternalOwnership GetExternalOwnership(const ImageState& image, uint32_t queueFamily) {
  if (!IsReleasedToExternal(image)) {
    return {};
  }
  return GetExternalOwnership(image.ExternalOwnerFamily, image.SharingMode, queueFamily);
}

ExternalOwnership GetExternalOwnership(const BufferState& buffer, uint32_t queueFamily) {
  if (!IsReleasedToExternal(buffer)) {
    return {};
  }
  return GetExternalOwnership(buffer.ExternalOwnerFamily, buffer.SharingMode, queueFamily);
}

void GetImageReadbackBarriers(VkImageLayout layout,
                              const ExternalOwnership& ownership,
                              VkImageMemoryBarrier& pre,
                              VkImageMemoryBarrier& post) {
  pre = {};
  pre.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  pre.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  pre.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  pre.oldLayout = layout;
  pre.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  pre.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  pre.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  post = pre;
  post.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  post.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  post.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  post.newLayout = layout;

  if (ownership.IsTransfer()) {
    // The source access of an acquire and the destination access of a release are ignored.
    pre.srcAccessMask = 0;
    pre.srcQueueFamilyIndex = ownership.ExternalFamily;
    pre.dstQueueFamilyIndex = ownership.QueueFamily;
    post.dstAccessMask = 0;
    post.srcQueueFamilyIndex = ownership.QueueFamily;
    post.dstQueueFamilyIndex = ownership.ExternalFamily;
  }
}

void GetBufferReadbackBarriers(const ExternalOwnership& ownership,
                               VkBufferMemoryBarrier& pre,
                               VkBufferMemoryBarrier& post) {
  pre = {};
  pre.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  pre.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  pre.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  pre.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  pre.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  pre.offset = 0;
  pre.size = VK_WHOLE_SIZE;

  post = pre;
  if (ownership.IsTransfer()) {
    pre.srcAccessMask = 0;
    pre.srcQueueFamilyIndex = ownership.ExternalFamily;
    pre.dstQueueFamilyIndex = ownership.QueueFamily;
    post.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    post.dstAccessMask = 0;
    post.srcQueueFamilyIndex = ownership.QueueFamily;
    post.dstQueueFamilyIndex = ownership.ExternalFamily;
  }
}

VkImageMemoryBarrier GetExternalReleaseBarrier(VkImageLayout oldLayout,
                                               VkImageLayout layout,
                                               const ExternalOwnership& ownership) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = layout;
  barrier.srcQueueFamilyIndex = ownership.QueueFamily;
  barrier.dstQueueFamilyIndex = ownership.ExternalFamily;
  return barrier;
}

VkBufferMemoryBarrier GetExternalReleaseBarrier(const ExternalOwnership& ownership) {
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;
  barrier.srcQueueFamilyIndex = ownership.QueueFamily;
  barrier.dstQueueFamilyIndex = ownership.ExternalFamily;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;
  return barrier;
}

std::optional<VkImageLayout> ImageLayoutFromGlLayout(uint32_t glLayout) {
  // The values of GL_EXT_semaphore, which maps each to the VkImageLayout of the same name.
  switch (glLayout) {
  case 0: // GL_NONE
    return VK_IMAGE_LAYOUT_UNDEFINED;
  case 0x958D: // GL_LAYOUT_GENERAL_EXT
    return VK_IMAGE_LAYOUT_GENERAL;
  case 0x958E: // GL_LAYOUT_COLOR_ATTACHMENT_EXT
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  case 0x958F: // GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  case 0x9590: // GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  case 0x9591: // GL_LAYOUT_SHADER_READ_ONLY_EXT
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  case 0x9592: // GL_LAYOUT_TRANSFER_SRC_EXT
    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  case 0x9593: // GL_LAYOUT_TRANSFER_DST_EXT
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  case 0x9530: // GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT
    return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
  case 0x9531: // GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT
    return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
  default:
    return std::nullopt;
  }
}

} // namespace vulkan
} // namespace gits
