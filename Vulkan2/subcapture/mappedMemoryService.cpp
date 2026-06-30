// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedMemoryService.h"
#include "stateTrackingService.h"
#include "objectState.h"
#include "commandsCustom.h"
#include "log.h"

#include <cstring>

namespace gits {
namespace vulkan {

MappedMemoryService::MappedMemoryService(StateTrackingService& sts) : m_StateTracking(sts) {}

void MappedMemoryService::OnMapMemory(uint64_t memoryKey,
                                      VkDeviceSize offset,
                                      VkDeviceSize size,
                                      VkMemoryMapFlags flags) {
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(memoryKey);
  if (state) {
    state->IsMapped = true;
    state->MappingOffset = offset;
    state->MappingSize = size;
    state->MappingFlags = flags;
  }
}

void MappedMemoryService::OnUnmapMemory(uint64_t memoryKey) {
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(memoryKey);
  if (state) {
    state->IsMapped = false;
    state->MappingOffset = 0;
    state->MappingSize = 0;
    state->MappingFlags = 0;
  }
}

void MappedMemoryService::OnMappedData(MappedDataMetaCommand& command) {
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(command.m_Memory.Key);
  if (!state || command.m_Regions.Regions.empty()) {
    return;
  }
  // Patch the shadow buffer in-place, matching DX12's shadowMemory_ approach.
  // region.Offset is relative to the mapped range start (MappingOffset within
  // the allocation), so the allocation-relative offset is MappingOffset + region.Offset.
  // Writing directly to the same position each time means only the latest data
  // for any byte range survives - no stale copies accumulate across frames.
  if (state->AllocationSize == 0) {
    return;
  }
  if (state->ShadowBuffer.empty()) {
    state->ShadowBuffer.assign(state->AllocationSize, 0u);
    state->ShadowDirtyBegin = state->AllocationSize;
    state->ShadowDirtyEnd = 0;
  }
  for (const auto& region : command.m_Regions.Regions) {
    const VkDeviceSize allocOffset = state->MappingOffset + region.Offset;
    if (allocOffset + region.Size > state->AllocationSize) {
      LOG_WARNING << "Vulkan2 subcapture: MappedDataMeta region [" << allocOffset << ", "
                  << allocOffset + region.Size << ") exceeds allocationSize "
                  << state->AllocationSize << " - skipping";
      continue;
    }
    std::memcpy(state->ShadowBuffer.data() + allocOffset, region.Data, region.Size);
    state->ShadowDirtyBegin = std::min(state->ShadowDirtyBegin, allocOffset);
    state->ShadowDirtyEnd = std::max(state->ShadowDirtyEnd, allocOffset + region.Size);
  }
}

} // namespace vulkan
} // namespace gits
