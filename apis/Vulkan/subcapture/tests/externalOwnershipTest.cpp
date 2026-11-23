// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the queue family ownership of resources released to an external queue
// family: which ones the content restore reads back, and the barriers it reads and restores them
// with. A wrong family or layout in these barriers is undefined behavior that drivers often
// tolerate, so it would surface only as corrupted contents on some of them.

#include "externalOwnership.h"
#include "objectState.h"

#include <cstdio>
#include <exception>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

using namespace gits::vulkan;

constexpr uint32_t kGraphicsFamily = 0;
constexpr uint32_t kTransferFamily = 2;
constexpr VkImageLayout kReleasedLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

// An EXCLUSIVE image whose whole range the application released to VK_QUEUE_FAMILY_EXTERNAL, as
// the image layout service leaves it once the release is submitted.
ImageState ExclusiveReleasedImage() {
  ImageState image;
  image.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image.ExclusiveOwnerFamily = UINT32_MAX;
  image.ExclusiveOwnershipPending = true;
  image.ExternalOwnerFamily = VK_QUEUE_FAMILY_EXTERNAL;
  image.CurrentLayout = kReleasedLayout;
  return image;
}

ImageState ConcurrentReleasedImage() {
  ImageState image;
  image.SharingMode = VK_SHARING_MODE_CONCURRENT;
  image.ConcurrentFamilies = {kGraphicsFamily, kTransferFamily};
  image.ExternalOwnerFamily = VK_QUEUE_FAMILY_EXTERNAL;
  image.CurrentLayout = kReleasedLayout;
  return image;
}

void TestExclusiveImageReleaseIsReadBack() {
  ImageState image = ExclusiveReleasedImage();
  Check(IsReleasedToExternal(image), "exclusive: a whole release to EXTERNAL is read back");

  image.ExternalOwnerFamily = VK_QUEUE_FAMILY_FOREIGN_EXT;
  Check(IsReleasedToExternal(image), "exclusive: a whole release to FOREIGN_EXT is read back");
}

void TestExclusiveImageOtherTransfersStayRejected() {
  ImageState vulkanTransfer = ExclusiveReleasedImage();
  vulkanTransfer.ExternalOwnerFamily = VK_QUEUE_FAMILY_IGNORED;
  Check(!IsReleasedToExternal(vulkanTransfer),
        "exclusive: a pending transfer between Vulkan families is not one to an external family");

  ImageState mixed = ExclusiveReleasedImage();
  mixed.ExclusiveOwnershipMixed = true;
  Check(!IsReleasedToExternal(mixed), "exclusive: a split image is not wholly released");

  ImageState partialVulkan = ExclusiveReleasedImage();
  partialVulkan.PartialOwnerStates = {PartialOwnerState::Released, PartialOwnerState::Untouched};
  Check(!IsReleasedToExternal(partialVulkan),
        "exclusive: an image in a partial transfer is not wholly released");

  ImageState partialExternal = ExclusiveReleasedImage();
  partialExternal.ExternalOwnershipPartial = true;
  Check(!IsReleasedToExternal(partialExternal),
        "exclusive: an image partly released to an external family is not wholly released");

  ImageState acquired = ExclusiveReleasedImage();
  acquired.ExclusiveOwnershipPending = false;
  acquired.ExclusiveOwnerFamily = kGraphicsFamily;
  Check(!IsReleasedToExternal(acquired),
        "exclusive: an image acquired back by Vulkan without the external owner being cleared "
        "is owned by Vulkan, not released");
}

void TestConcurrentImageRelease() {
  ImageState image = ConcurrentReleasedImage();
  Check(IsReleasedToExternal(image), "concurrent: a whole release to EXTERNAL is read back");

  image.ExternalOwnershipPartial = true;
  Check(!IsReleasedToExternal(image), "concurrent: a partial release is not");

  ImageState owned = ConcurrentReleasedImage();
  owned.ExternalOwnerFamily = VK_QUEUE_FAMILY_IGNORED;
  Check(!IsReleasedToExternal(owned), "concurrent: an image Vulkan owns needs no transfer");
  Check(!GetExternalOwnership(owned, kGraphicsFamily).IsTransfer(),
        "concurrent: no transfer for an image Vulkan owns");
}

void TestBufferRelease() {
  BufferState buffer;
  buffer.ExternalOwnerFamily = VK_QUEUE_FAMILY_EXTERNAL;
  Check(IsReleasedToExternal(buffer), "buffer: a whole release is read back");
  const ExternalOwnership exclusive = GetExternalOwnership(buffer, kTransferFamily);
  Check(exclusive.ExternalFamily == VK_QUEUE_FAMILY_EXTERNAL &&
            exclusive.QueueFamily == kTransferFamily,
        "buffer: an EXCLUSIVE buffer transfers onto the readback family");

  buffer.SharingMode = VK_SHARING_MODE_CONCURRENT;
  Check(GetExternalOwnership(buffer, kTransferFamily).QueueFamily == VK_QUEUE_FAMILY_IGNORED,
        "buffer: a CONCURRENT buffer names only the external family");

  buffer.ExternalOwnershipPartial = true;
  Check(!IsReleasedToExternal(buffer), "buffer: a partial release is not read back");
  Check(!GetExternalOwnership(buffer, kTransferFamily).IsTransfer(),
        "buffer: no transfer for a partly released buffer");
}

// The barriers of an image Vulkan owns must stay exactly as they were before external ownership
// was handled.
void TestReadbackBarriersWithoutTransfer() {
  VkImageMemoryBarrier pre{};
  VkImageMemoryBarrier post{};
  GetImageReadbackBarriers(kReleasedLayout, ExternalOwnership{}, pre, post);
  Check(pre.sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, "no transfer: pre sType");
  Check(pre.srcAccessMask == VK_ACCESS_MEMORY_WRITE_BIT &&
            pre.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT,
        "no transfer: pre access");
  Check(pre.oldLayout == kReleasedLayout && pre.newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        "no transfer: pre layouts");
  Check(pre.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            pre.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED,
        "no transfer: pre families");
  Check(post.srcAccessMask == VK_ACCESS_TRANSFER_READ_BIT &&
            post.dstAccessMask == (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
        "no transfer: post access");
  Check(post.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && post.newLayout == kReleasedLayout,
        "no transfer: post layouts");
  Check(post.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            post.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED,
        "no transfer: post families");

  VkBufferMemoryBarrier bufferPre{};
  VkBufferMemoryBarrier bufferPost{};
  GetBufferReadbackBarriers(ExternalOwnership{}, bufferPre, bufferPost);
  Check(bufferPre.srcAccessMask == VK_ACCESS_MEMORY_WRITE_BIT &&
            bufferPre.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT &&
            bufferPre.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            bufferPre.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED && bufferPre.offset == 0 &&
            bufferPre.size == VK_WHOLE_SIZE,
        "no transfer: buffer barrier unchanged");
}

void TestReadbackBarriersAcquireAndReleaseExclusive() {
  const ImageState image = ExclusiveReleasedImage();
  const ExternalOwnership ownership = GetExternalOwnership(image, kTransferFamily);
  Check(ownership.IsTransfer(), "exclusive readback: transfers");

  VkImageMemoryBarrier pre{};
  VkImageMemoryBarrier post{};
  GetImageReadbackBarriers(image.CurrentLayout, ownership, pre, post);
  Check(pre.srcQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
            pre.dstQueueFamilyIndex == kTransferFamily,
        "exclusive readback: acquires from EXTERNAL onto the readback family");
  Check(pre.oldLayout == kReleasedLayout && pre.newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        "exclusive readback: acquires from the layout of the release");
  Check(pre.srcAccessMask == 0 && pre.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT,
        "exclusive readback: acquire access");
  Check(post.srcQueueFamilyIndex == kTransferFamily &&
            post.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL,
        "exclusive readback: releases back to EXTERNAL");
  Check(post.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && post.newLayout == kReleasedLayout,
        "exclusive readback: releases in the layout of the release");
  Check(post.srcAccessMask == VK_ACCESS_TRANSFER_READ_BIT && post.dstAccessMask == 0,
        "exclusive readback: release access");
}

void TestReadbackBarriersAcquireAndReleaseConcurrent() {
  const ImageState image = ConcurrentReleasedImage();
  VkImageMemoryBarrier pre{};
  VkImageMemoryBarrier post{};
  GetImageReadbackBarriers(image.CurrentLayout, GetExternalOwnership(image, kGraphicsFamily), pre,
                           post);
  // VUID-VkImageMemoryBarrier-image-04071
  Check(pre.srcQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
            pre.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED,
        "concurrent readback: acquire names only the external family");
  Check(post.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            post.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL,
        "concurrent readback: release names only the external family");

  BufferState buffer;
  buffer.SharingMode = VK_SHARING_MODE_CONCURRENT;
  buffer.ExternalOwnerFamily = VK_QUEUE_FAMILY_EXTERNAL;
  VkBufferMemoryBarrier bufferPre{};
  VkBufferMemoryBarrier bufferPost{};
  GetBufferReadbackBarriers(GetExternalOwnership(buffer, kGraphicsFamily), bufferPre, bufferPost);
  Check(bufferPre.srcQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
            bufferPre.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            bufferPre.srcAccessMask == 0,
        "concurrent buffer readback: acquire");
  Check(bufferPost.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            bufferPost.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
            bufferPost.srcAccessMask == VK_ACCESS_TRANSFER_READ_BIT &&
            bufferPost.dstAccessMask == 0,
        "concurrent buffer readback: release");
}

void TestRestoreRelease() {
  const ImageState image = ExclusiveReleasedImage();
  const ExternalOwnership ownership = GetExternalOwnership(image, kGraphicsFamily);

  // Uploaded: the copy left the image in its layout on the family that restored it.
  const VkImageMemoryBarrier uploaded =
      GetExternalReleaseBarrier(image.CurrentLayout, image.CurrentLayout, ownership);
  Check(uploaded.oldLayout == kReleasedLayout && uploaded.newLayout == kReleasedLayout,
        "restore: an uploaded image is released in its layout without a transition");
  Check(uploaded.srcQueueFamilyIndex == kGraphicsFamily &&
            uploaded.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL,
        "restore: released from the restoring family to EXTERNAL");

  // Not uploaded: the layout restore transitions it from UNDEFINED as part of the release.
  const VkImageMemoryBarrier notUploaded =
      GetExternalReleaseBarrier(VK_IMAGE_LAYOUT_UNDEFINED, image.CurrentLayout, ownership);
  Check(notUploaded.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            notUploaded.newLayout == kReleasedLayout,
        "restore: an image without contents is released into its layout");

  const VkBufferMemoryBarrier buffer = GetExternalReleaseBarrier(ownership);
  Check(buffer.srcQueueFamilyIndex == kGraphicsFamily &&
            buffer.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL && buffer.offset == 0 &&
            buffer.size == VK_WHOLE_SIZE,
        "restore: a buffer is released whole to EXTERNAL");
}

void TestGlLayouts() {
  struct Mapping {
    uint32_t GlLayout;
    VkImageLayout Layout;
  };
  const Mapping mappings[] = {
      {0, VK_IMAGE_LAYOUT_UNDEFINED},
      {0x958D, VK_IMAGE_LAYOUT_GENERAL},
      {0x958E, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {0x958F, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
      {0x9590, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
      {0x9591, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {0x9592, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
      {0x9593, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL},
      {0x9530, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL},
      {0x9531, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL},
  };
  for (const Mapping& mapping : mappings) {
    const auto layout = ImageLayoutFromGlLayout(mapping.GlLayout);
    Check(layout.has_value() && *layout == mapping.Layout,
          "GL layout: maps to the VkImageLayout GL_EXT_semaphore names");
  }
  Check(!ImageLayoutFromGlLayout(0x1234).has_value(), "GL layout: an unknown one has no layout");
}

} // namespace

int main() {
  try {
    TestExclusiveImageReleaseIsReadBack();
    TestExclusiveImageOtherTransfersStayRejected();
    TestConcurrentImageRelease();
    TestBufferRelease();
    TestReadbackBarriersWithoutTransfer();
    TestReadbackBarriersAcquireAndReleaseExclusive();
    TestReadbackBarriersAcquireAndReleaseConcurrent();
    TestRestoreRelease();
    TestGlLayouts();
  } catch (const std::exception& e) {
    std::printf("externalOwnershipTest: unexpected exception: %s\n", e.what());
    return 1;
  } catch (...) {
    std::printf("externalOwnershipTest: unexpected exception\n");
    return 1;
  }

  if (g_Failures != 0) {
    std::printf("externalOwnershipTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("externalOwnershipTest: all checks passed\n");
  return 0;
}
