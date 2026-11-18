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
#include <optional>

namespace gits {
namespace vulkan {

struct BufferState;
struct ImageState;

// Queue family ownership of resources that the application releases to a queue family outside
// Vulkan, such as the OpenGL driver an exported allocation is shared with. Vulkan may access such
// a resource only after acquiring it back, so the content readback acquires it and releases it
// again, and the state restore ends with it released.

// VK_QUEUE_FAMILY_EXTERNAL or VK_QUEUE_FAMILY_FOREIGN_EXT.
bool IsExternalQueueFamily(uint32_t family);

// Whether the whole resource is released to an external queue family: an EXCLUSIVE image must
// also be in that one transfer, not in a partial or Vulkan-to-Vulkan one.
bool IsReleasedToExternal(const ImageState& image);
bool IsReleasedToExternal(const BufferState& buffer);

// The queue families of the transfer between an external family and the queue a resource is read
// or restored on. ExternalFamily is VK_QUEUE_FAMILY_IGNORED for a resource Vulkan owns, which
// needs no transfer. QueueFamily is VK_QUEUE_FAMILY_IGNORED for a VK_SHARING_MODE_CONCURRENT
// resource, as its transfers name only the external family
// (VUID-VkImageMemoryBarrier-image-04071).
struct ExternalOwnership {
  uint32_t ExternalFamily{VK_QUEUE_FAMILY_IGNORED};
  uint32_t QueueFamily{VK_QUEUE_FAMILY_IGNORED};

  bool IsTransfer() const {
    return IsExternalQueueFamily(ExternalFamily);
  }
};

ExternalOwnership GetExternalOwnership(const ImageState& image, uint32_t queueFamily);
ExternalOwnership GetExternalOwnership(const BufferState& buffer, uint32_t queueFamily);

// The barriers before and after a readback copy of an image in `layout`. With a transfer, `pre`
// acquires the image from the external family into TRANSFER_SRC_OPTIMAL and `post` releases it
// back in `layout`, in which the external owner expects it.
void GetImageReadbackBarriers(VkImageLayout layout,
                              const ExternalOwnership& ownership,
                              VkImageMemoryBarrier& pre,
                              VkImageMemoryBarrier& post);
// The barriers before and after a readback copy of a buffer. `post` is needed only for a
// transfer, which it releases back.
void GetBufferReadbackBarriers(const ExternalOwnership& ownership,
                               VkBufferMemoryBarrier& pre,
                               VkBufferMemoryBarrier& post);

// The release of a restored image or buffer to the external family, from `oldLayout` into the
// `layout` it was released in. Without source access, as the state restore writes these after the
// uploads have completed and made their writes available.
VkImageMemoryBarrier GetExternalReleaseBarrier(VkImageLayout oldLayout,
                                               VkImageLayout layout,
                                               const ExternalOwnership& ownership);
VkBufferMemoryBarrier GetExternalReleaseBarrier(const ExternalOwnership& ownership);

// The VkImageLayout of an OpenGL layout of GL_EXT_semaphore; nullopt for an unknown one.
std::optional<VkImageLayout> ImageLayoutFromGlLayout(uint32_t glLayout);

} // namespace vulkan
} // namespace gits
