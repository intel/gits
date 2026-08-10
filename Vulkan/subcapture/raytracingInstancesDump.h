// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "stateTrackingService.h"

#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

// Analysis-pass helper: reads back a TLAS's instance buffer from the live GPU and
// resolves each VkAccelerationStructureInstanceKHR::accelerationStructureReference to
// the BLAS object key it points at, via DeviceAddressTrackingService.
class RaytracingInstancesDump {
public:
  RaytracingInstancesDump(StateTrackingService& stateTracking, IGpuReadbackHelper& gpuReadback)
      : m_StateTracking(stateTracking), m_GpuReadback(gpuReadback) {}

  // Returns the deduplicated BLAS keys referenced by a TLAS build's instance data at
  // instanceArrayAddress. Empty if that data could not be located or read.
  // arrayOfPointers picks the layout: `instanceCount` contiguous
  // VkAccelerationStructureInstanceKHR structs when false, `instanceCount`
  // VkDeviceAddress pointers to structs elsewhere when true.
  std::vector<uint64_t> ReadReferencedBlases(uint64_t deviceKey,
                                             uint64_t physDevKey,
                                             uint64_t queueKey,
                                             uint64_t poolKey,
                                             VkDeviceAddress instanceArrayAddress,
                                             uint32_t instanceCount,
                                             bool arrayOfPointers);

private:
  StateTrackingService& m_StateTracking;
  IGpuReadbackHelper& m_GpuReadback;
};

} // namespace vulkan
} // namespace gits
