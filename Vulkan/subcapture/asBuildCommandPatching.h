// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <cstdint>
#include <unordered_set>

namespace gits {
namespace vulkan {

// Drops every info whose destination is not in keepDstAsKeys - structures the analysis found
// unused in the subcaptured range - keeping infoCount, pInfos, its handle keys and
// ppBuildRangeInfos in step.
//
// Returns false and leaves cmd untouched when pInfos.HandleKeys does not have the recorded
// layout (all [src, dst] pairs, then each info's variable length pNext payload). Callers must
// treat that as fatal - guessing at the split hands wrong handles to the driver.
bool RemoveUnreferencedAsBuildInfos(vkCmdBuildAccelerationStructuresKHRCommand& cmd,
                                    const std::unordered_set<uint64_t>& keepDstAsKeys);

} // namespace vulkan
} // namespace gits
