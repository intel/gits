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

namespace gits {
namespace vulkan {

class StateTrackingService;
class MappedDataMetaCommand;

// Tracks the mapped state and shadow-buffer contents of VkDeviceMemory objects.
//
// Extracted from SubcaptureLayer to isolate the three related tracking points:
//   - vkMapMemory   ? OnMapMemory
//   - vkUnmapMemory ? OnUnmapMemory
//   - MappedDataMetaCommand ? OnMappedData (shadow buffer patching)
//
// The shadow buffer mirrors the host-visible memory contents between map/unmap
// cycles so that state restore can re-emit MappedDataMetaCommands that replay
// the correct initial memory contents in the subcapture player.
class MappedMemoryService {
public:
  explicit MappedMemoryService(StateTrackingService& sts);

  // Called after a successful vkMapMemory: records offset/size/flags on the state.
  void OnMapMemory(uint64_t memoryKey,
                   VkDeviceSize offset,
                   VkDeviceSize size,
                   VkMemoryMapFlags flags);

  // Called after vkUnmapMemory: clears the mapped fields on the state.
  void OnUnmapMemory(uint64_t memoryKey);

  // Called after MappedDataMetaCommand: patches the shadow buffer with the
  // regions that the player just wrote into the mapped host pointer.
  void OnMappedData(MappedDataMetaCommand& command);

private:
  StateTrackingService& m_StateTracking;
};

} // namespace vulkan
} // namespace gits
