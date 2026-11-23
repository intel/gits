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
#include <vector>

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

// The VkAccelerationStructureTrianglesOpacityMicromapEXT extending a triangles geometry, if
// any. The non-const overload is for the decoded scratch copy a restore path mutates.
const VkAccelerationStructureTrianglesOpacityMicromapEXT* FindOpacityMicromapGeometry(
    const void* pNext);
VkAccelerationStructureTrianglesOpacityMicromapEXT* FindOpacityMicromapGeometry(void* pNext);

// Appends the GITSKey of every micromap this build names in a pNext chain - false on a bad layout.
bool CollectAsBuildMicromapKeys(const vkCmdBuildAccelerationStructuresKHRCommand& cmd,
                                std::vector<uint64_t>& outMicromapKeys);

// The micromap counterpart of RemoveUnreferencedAsBuildInfos. Far simpler: HandleKeys[i] is
// info i's dstMicromap, one key per info with no payload run. Returns false when
// HandleKeys.size() != infoCount, which callers must treat as fatal.
bool RemoveUnreferencedMicromapBuildInfos(vkCmdBuildMicromapsEXTCommand& cmd,
                                          const std::unordered_set<uint64_t>& keepDstMicromapKeys);

} // namespace vulkan
} // namespace gits
