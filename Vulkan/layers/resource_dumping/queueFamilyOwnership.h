// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>

namespace gits {
namespace vulkan {

// The queue family ownership of a resource created with VK_SHARING_MODE_EXCLUSIVE.
struct Ownership {
  enum class State {
    // No tracked use or transfer established an owner.
    Unknown,
    Owned,
    // Released to QueueFamilyIndex, which has not acquired it yet, so no queue family owns it.
    Released,
    // Parts of the resource have different owners, or are released while other parts are not.
    Mixed,
  };
  State Is{State::Unknown};
  // The owning queue family, or the one the resource is released to.
  uint32_t QueueFamilyIndex{};
};

// A use of a resource on a queue family, or one side of a queue family ownership transfer, as
// executed by a command buffer.
struct OwnershipChange {
  enum class Kind {
    // An access without a transfer, which takes ownership of the accessed part.
    Use,
    // The release half of a transfer, executed on the source queue family.
    Release,
    // The acquire half of a transfer, executed on the destination queue family.
    Acquire,
  };
  Kind Is{};
  // The using queue family, or the destination queue family of a transfer.
  uint32_t QueueFamilyIndex{};
  // The use or transfer covers every subresource or byte of the resource.
  bool Whole{};
};

void ApplyOwnershipChange(Ownership& ownership, const OwnershipChange& change);

} // namespace vulkan
} // namespace gits
