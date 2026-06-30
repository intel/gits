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
    state->isMapped = true;
    state->mappingOffset = offset;
    state->mappingSize = size;
    state->mappingFlags = flags;
  }
}

void MappedMemoryService::OnUnmapMemory(uint64_t memoryKey) {
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(memoryKey);
  if (state) {
    state->isMapped = false;
    state->mappingOffset = 0;
    state->mappingSize = 0;
    state->mappingFlags = 0;
  }
}

void MappedMemoryService::OnMappedData(MappedDataMetaCommand& command) {
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(command.m_Memory.Key);
  if (!state || command.m_Regions.Regions.empty()) {
    return;
  }
  // Patch the shadow buffer in-place, matching DX12's shadowMemory_ approach.
  // region.Offset is relative to the mapped range start (mappingOffset within
  // the allocation), so the allocation-relative offset is mappingOffset + region.Offset.
  // Writing directly to the same position each time means only the latest data
  // for any byte range survives - no stale copies accumulate across frames.
  if (state->allocationSize == 0) {
    return;
  }
  if (state->shadowBuffer.empty()) {
    state->shadowBuffer.assign(state->allocationSize, 0u);
    state->shadowDirtyBegin = state->allocationSize;
    state->shadowDirtyEnd = 0;
  }
  for (const auto& region : command.m_Regions.Regions) {
    const VkDeviceSize allocOffset = state->mappingOffset + region.Offset;
    if (allocOffset + region.Size > state->allocationSize) {
      LOG_WARNING << "Vulkan2 subcapture: MappedDataMeta region [" << allocOffset << ", "
                  << allocOffset + region.Size << ") exceeds allocationSize "
                  << state->allocationSize << " - skipping";
      continue;
    }
    std::memcpy(state->shadowBuffer.data() + allocOffset, region.Data, region.Size);
    state->shadowDirtyBegin = std::min(state->shadowDirtyBegin, allocOffset);
    state->shadowDirtyEnd = std::max(state->shadowDirtyEnd, allocOffset + region.Size);
  }
}

} // namespace vulkan
} // namespace gits
