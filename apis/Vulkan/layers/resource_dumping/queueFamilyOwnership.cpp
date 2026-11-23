// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "queueFamilyOwnership.h"

namespace gits {
namespace vulkan {

void ApplyOwnershipChange(Ownership& ownership, const OwnershipChange& change) {
  const Ownership owned{Ownership::State::Owned, change.QueueFamilyIndex};
  const bool ownedByChangeFamily = ownership.Is == Ownership::State::Owned &&
                                   ownership.QueueFamilyIndex == change.QueueFamilyIndex;
  switch (change.Is) {
  case OwnershipChange::Kind::Use:
    if (change.Whole || ownership.Is == Ownership::State::Unknown || ownedByChangeFamily) {
      ownership = owned;
    } else {
      ownership.Is = Ownership::State::Mixed;
    }
    break;
  case OwnershipChange::Kind::Release:
    if (change.Whole) {
      ownership = {Ownership::State::Released, change.QueueFamilyIndex};
    } else {
      ownership.Is = Ownership::State::Mixed;
    }
    break;
  case OwnershipChange::Kind::Acquire:
    if (change.Whole || ownedByChangeFamily) {
      ownership = owned;
    } else {
      ownership.Is = Ownership::State::Mixed;
    }
    break;
  }
}

} // namespace vulkan
} // namespace gits
